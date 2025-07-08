/**
 * Esp32 C3 E-ink Weather Board
 * Alpatov Danila <alpatovdanila@gmail.com>
 */

#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
//#include <GxEPD2_4G_4G.h>
#include <GxEPD2_4G_BW.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMono9pt7b.h>

#define EPD_MOSI          GPIO_NUM_6
#define EPD_SCK           GPIO_NUM_7
#define EPD_CS            GPIO_NUM_2
#define EPD_DC            GPIO_NUM_3
#define EPD_RST           GPIO_NUM_1
#define EPD_BUSY          GPIO_NUM_10


#define IS_DEBUG          true

#define WIFI_TIMEOUT_MS   10000

#define BUTTON_PIN        GPIO_NUM_4

#define SLEEP_TIME        60 * 60 * 1000 * 1000ULL

#define WIFI_SSID         "kv682 4g"
#define WIFI_PASS         "zxcvbnm1"
#define OWM_API_KEY       "c2b7eca69ccd2f6872979804bd5f35f7"
#define CITY_NAME         "Moscow"

#include <select_display.h>

struct Weather {
  int8_t  temp;
  int8_t  feels_like;
  int8_t  temp_in_1h;
  uint16_t condition;
};

enum class Status { WifiFailed, FetchFailed, Fetching, Connecting, Updated, None };

static void goSleep()
{
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  display.hibernate();

  esp_deep_sleep_enable_gpio_wakeup(1ULL << GPIO_NUM_4, ESP_GPIO_WAKEUP_GPIO_LOW);
  esp_sleep_enable_timer_wakeup(SLEEP_TIME);
  esp_deep_sleep_start();
}

Weather* fetchWeather()
{
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
  const char* json = body.c_str();
  
  DeserializationError error = deserializeJson(doc, json);

  if (error) return nullptr;

  Weather* result = new Weather();

  result->temp = int8_t(doc["main"]["temp"].as<float>());
  result->feels_like = int8_t(doc["main"]["feels_like"].as<float>());
  result->temp_in_1h = int8_t(doc["main"]["temp"].as<float>());
  result->condition = doc["weather"][0]["id"].as<uint16_t>();

  return result;
}


bool retrieveWifiStatus()
{
  unsigned long startAttemptTime = millis();

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  const uint32_t t0 = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT_MS) {
    delay(100);
  }

  return WiFi.status() == WL_CONNECTED;
}



void render(Weather* weather)
{
  display.firstPage();
  do {
    display.setFont(&FreeMonoBold24pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(20, 150);
    
    display.print(weather->temp);
    display.print("°C");
  
  } while (display.nextPage());
}

void renderSplash(){
  display.firstPage();
  display.fillScreen(GxEPD_WHITE);
  display.setCursor(20, 150);
  display.print("Weather Board");
  display.display(true);
}

void renderStatus(Status status)
{   
    display.setFont(&FreeMono9pt7b);

    display.setTextColor(GxEPD_BLACK);
    
    display.firstPage();
    do{

      display.fillScreen(GxEPD_WHITE);

      display.setCursor(0,12);

      if (status == Status::WifiFailed) display.print("WiFi connection failed");
      if (status == Status::Fetching) display.print("Fetching weather data...");
      if (status == Status::Connecting) display.print("Connecting to WiFi...");
      if (status == Status::Updated) display.print("Updated successfully");

    }while(display.nextPage());
}

void mainRoutine(){
  //renderStatus(Status::Connecting);

  const bool wifiStatus = retrieveWifiStatus();

  if(!wifiStatus){
    renderStatus(Status::WifiFailed);
    return;
  }

  //renderStatus(Status::Fetching);

  Weather* weather  = fetchWeather();

  if(!weather){
    //renderStatus(Status::FetchFailed);
    return;
  }

  //renderStatus(Status::Updated);
  
  render(weather);
}

void setup() 
{
  Serial.begin(115200);
  while (!Serial);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  SPI.begin(EPD_SCK, -1, EPD_MOSI, EPD_CS);

  display.init(115200, false, 20, false);

  display.setRotation(2); 

  renderSplash();

  mainRoutine();

  //goSleep();
}

void loop() {
  #if IS_DEBUG
  
  static bool wasDown = false;
  bool nowDown = (digitalRead(BUTTON_PIN) == LOW);
  
  if (nowDown && !wasDown) {
    display.display(false);
  }         
  
  wasDown = nowDown;

  #endif
}