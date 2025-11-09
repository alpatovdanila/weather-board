/**
 * Esp32 C3 E-ink Weather Board
 * Alpatov Danila <alpatovdanila@gmail.com>
 */

#include <Arduino.h>
#include <ArduinoJson.h>

#include <Bahamas88.h>

#include <FreeMonoBold10.h>
#include <FreeMonoBold18.h>
#include <GxEPD2_4G_4G.h>
// #include <GxEPD2_4G_BW.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <WiFi.h>
#include <weather_icons/icons.h>

#define EPD_MOSI GPIO_NUM_6
#define EPD_SCK GPIO_NUM_7
#define EPD_CS GPIO_NUM_2
#define EPD_DC GPIO_NUM_3
#define EPD_RST GPIO_NUM_1
#define EPD_BUSY GPIO_NUM_10

#define IS_DEBUG true

#define WIFI_TIMEOUT_MS 10000

#define BUTTON_PIN GPIO_NUM_4

#define SLEEP_TIME 60 * 60 * 1000 * 1000ULL // 1 hour in microseconds

#define CITY_NAME "Moscow"

#define LATITUDE 55.919156
#define LONGITUDE 37.715051

#include <secrets.h>

#include <select_display.h>

struct Weather {
  int8_t temp;
  int8_t feels_like;
  int8_t temp_in_1h;
  String icon;
  String condition;
};

enum class Status {
  Started,
  WifiFailed,
  FetchFailed,
  Fetching,
  Connecting,
  Updated,
  None
};


static void goSleep(uint64_t sleepTimeUs = SLEEP_TIME) {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  display.hibernate();

  esp_deep_sleep_enable_gpio_wakeup(1ULL << GPIO_NUM_4,
                                    ESP_GPIO_WAKEUP_GPIO_LOW);

  esp_sleep_enable_timer_wakeup(sleepTimeUs);
  esp_deep_sleep_start();
}

Weather *fetchWeather() {
  const String url =
      "https://api.openweathermap.org/data/2.5/weather?lang=ru&lat=" +
      String(LATITUDE) + "&lon=" + String(LONGITUDE) +
      "&units=metric&appid=" + OWM_API_KEY;

  HTTPClient http;

  http.begin(url);

  int code = http.GET();

  if (code != 200) {
    http.end();
    return nullptr;
  }

  String body = http.getString();
  http.end();

  StaticJsonDocument<200> doc;
  const char *json = body.c_str();

  DeserializationError error = deserializeJson(doc, json);

  if (error)
    return nullptr;

  Weather *result = new Weather();

  result->temp = int8_t(doc["main"]["temp"].as<float>());
  result->feels_like = int8_t(doc["main"]["feels_like"].as<float>());
  result->icon = doc["weather"][0]["icon"].as<String>();
  result->condition = doc["weather"][0]["description"].as<String>();

  return result;
}

void drawWeatherIcon(const String &code, int16_t x, int16_t y) {
  const IconEntry *icon = getIconByCode(code);
  if (icon) {
    display.drawBitmap(x, y, icon->data, icon->width, icon->height,
                       GxEPD_BLACK);
  }
}

bool connectWifi() {
  unsigned long startAttemptTime = millis();

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startAttemptTime < WIFI_TIMEOUT_MS) {
    delay(100);
  }

  return WiFi.status() == WL_CONNECTED;
}


uint16_t leftBoundForCentered(uint16_t itemWidth, uint16_t centerX) {
  return centerX - itemWidth / 2;
}

void render(Weather *weather) {

  auto tempText = String(weather->temp);
  auto tempFeelText = String(weather->feels_like);
  auto conditionText = weather->condition;

  auto iconWidth = 128;
  auto iconHeight = iconWidth;
  auto screenWidth = display.width();
  auto screenHeight = display.height();
  auto topSectionHeight = screenHeight / 2 + 32;

  display.firstPage();

  do {
    display.setFont(&Bahamas88pt8b);
    display.setTextColor(GxEPD_WHITE);

    int16_t _tbx, _tby;
    uint16_t tbw, tbh;

    display.fillRect(0, 0, screenWidth, topSectionHeight, GxEPD_BLACK);
    display.getTextBounds(tempText, 35, 65, &_tbx, &_tby, &tbw, &tbh);
    display.setCursor(leftBoundForCentered(tbw, screenWidth / 2) - 16, 175);
    display.print(tempText);

    display.setFont(&FreeMonoBold18pt8b);
    display.setCursor(leftBoundForCentered(tbw, screenWidth / 2) + tbw, 70);
    display.print("o");

    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeMonoBold10pt8b);
    display.getTextBounds(conditionText, 0, 0, &_tbx, &_tby, &tbw, &tbh);

    drawWeatherIcon(weather->icon,
                    leftBoundForCentered(iconWidth, screenWidth / 2),
                    topSectionHeight + 8);

    display.setCursor(leftBoundForCentered(tbw, screenWidth / 2),
                      screenHeight - 16);

    display.print(weather->condition);

  } while (display.nextPage());
}

void renderStatusImpl(Status status) {
  display.setTextColor(GxEPD_BLACK);

  display.setPartialWindow(10, 10, 50, 50);

  display.firstPage();
  do {

    display.setCursor(0, 12);

    if (status == Status::WifiFailed)
      display.print("WiFi connection failed");

    if (status == Status::Fetching)
      display.print("Fetching weather data");

    if (status == Status::Connecting)
      display.print("Connecting to WiFi");

    if (status == Status::Updated)
      display.print("Updated successfully");

  } while (display.nextPage());
}

void renderStatusImpl2(Status status) {
  if (status == Status::Started)
    Serial.println("Started");

  if (status == Status::WifiFailed)
    Serial.println("WiFi connection failed");

  if (status == Status::Fetching)
    Serial.println("Fetching weather data");

  if (status == Status::Connecting)
    Serial.println("Connecting to WiFi");

  if (status == Status::Updated)
    Serial.println("Updated successfully");

}

void renderStatus(Status status) { renderStatusImpl2(status); }

void mainRoutine() {
  renderStatus(Status::Connecting);
  const bool wifiStatus = connectWifi();

  if (!wifiStatus) {
    renderStatus(Status::WifiFailed);
    return;
  }

  renderStatus(Status::Fetching);
  Weather *weather = fetchWeather();

  if (!weather) {
    renderStatus(Status::FetchFailed);
    return;
  }

  renderStatus(Status::Updated);

  render(weather);
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  renderStatus(Status::Started);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  SPI.begin(EPD_SCK, -1, EPD_MOSI, -1);

  display.init(115200);

  display.setRotation(1);

  mainRoutine();

  goSleep();
}

void loop() {
#if IS_DEBUG

  static bool wasDown = false;
  bool nowDown = (digitalRead(BUTTON_PIN) == LOW);

  if (nowDown && !wasDown) {
    mainRoutine();
  }

  wasDown = nowDown;

#endif
}
