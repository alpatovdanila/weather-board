/**
 * Esp32 C3 E-ink Weather Board
 * Alpatov Danila <alpatovdanila@gmail.com>
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ComfortaaNumbers88pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <GxEPD2_4G_4G.h>
// #include <GxEPD2_4G_BW.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <WiFi.h>

#define EPD_MOSI GPIO_NUM_6
#define EPD_SCK GPIO_NUM_7
#define EPD_CS GPIO_NUM_2
#define EPD_DC GPIO_NUM_3
#define EPD_RST GPIO_NUM_1
#define EPD_BUSY GPIO_NUM_10

#define IS_DEBUG true

#define WIFI_TIMEOUT_MS 10000

#define BUTTON_PIN GPIO_NUM_4

#define SLEEP_TIME 60 * 60 * 1000 * 1000ULL

#define CITY_NAME "Moscow"

#include <secrets.h>

#include <select_display.h>

struct Weather {
  int8_t temp;
  int8_t feels_like;
  int8_t temp_in_1h;
  uint16_t condition;
};

enum class Status {
  WifiFailed,
  FetchFailed,
  Fetching,
  Connecting,
  Updated,
  SyncTime,
  None
};

static void goSleep() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  display.hibernate();

  esp_deep_sleep_enable_gpio_wakeup(1ULL << GPIO_NUM_4,
                                    ESP_GPIO_WAKEUP_GPIO_LOW);

  esp_sleep_enable_timer_wakeup(SLEEP_TIME);
  esp_deep_sleep_start();
}

Weather *fetchWeather() {
  const String url =
      "https://api.openweathermap.org/data/2.5/weather?q=" CITY_NAME
      "&units=metric&appid=" OWM_API_KEY;

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
  result->temp_in_1h = int8_t(doc["main"]["temp"].as<float>());
  result->condition = doc["weather"][0]["id"].as<uint16_t>();

  return result;
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

void render(Weather *weather) {

  auto tempText = String(weather->temp);

  display.firstPage();
  do {
    display.setFont(&ComfortaaNumbers88pt7b);
    display.setTextColor(GxEPD_BLACK);

    int16_t tbx, tby;
    uint16_t tbw, tbh;

    display.getTextBounds(tempText, 80, -150, &tbx, &tby, &tbw, &tbh);
    display.setCursor(tbx, tby);
    display.print(tempText);

    display.setFont(&FreeMonoBold24pt7b);
    display.setCursor(tbx + tbw + 16, 115);
    display.print("o");

  } while (display.nextPage());
}

void renderStatus(Status status) {

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

    if (status == Status::SyncTime)
      display.print("Syncing time");

  } while (display.nextPage());
}

void mainRoutine() {
  // renderStatus(Status::Connecting);
  const bool wifiStatus = connectWifi();

  if (!wifiStatus) {
    // renderStatus(Status::WifiFailed);
    return;
  }

  // renderStatus(Status::SyncTime);
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  // renderStatus(Status::Fetching);
  Weather *weather = fetchWeather();

  if (!weather) {
    // renderStatus(Status::FetchFailed);
    return;
  }

  // renderStatus(Status::Updated);

  render(weather);
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  SPI.begin(EPD_SCK, -1, EPD_MOSI, -1);

  display.init(115200);

  display.setRotation(2);

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