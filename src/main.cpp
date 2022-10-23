#ifdef __AVR__
  #include <avr/pgmspace.h>
#else
  #include <pgmspace.h>
#endif

#include <SPI.h>
#include <TFT_eSPI.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>

#include <pin_config.h>
#include "resources/images/animation.h"
#include "resources/fonts/orbitron_medium_20.h"

#define TFT_GREY 0x5AEB
#define lightblue 0x01E9
#define darkred 0xA041
#define blue 0x5D9B
#define HOUR 3600
#define GMT_OFFSET 2

const int pwmFreq = 5000;
const int pwmResolution = 8;
const int pwmLedChannelTFT = 0;

/** 
 * Configuration data from config.cpp
 * 
 * Copy config.cpp.template => config.cpp
 * Edit the contents with the values that you want to use
 */
extern const char *town;
extern const char *country;
extern const char *ssid;
extern const char *password;
extern const char *api_key;
extern const char *endpoint;

const char *url = NULL;
String payload = ""; // whole json
String tmp = "";     // temperature
String hum = "";     // humidity

StaticJsonDocument<1000> doc;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

int backlight[5] = {10, 30, 60, 120, 220};
byte b = 4;

TFT_eSPI tft = TFT_eSPI();

const char * get_endpoint(const char *endpoint, const char *town, const char *country, const char *api_key)
{
  const char *format = "%s?q=%s,%s&units=metric&appid=%s";

  int format_len = strlen(format);
  int endpoint_len = strlen(endpoint);
  int town_len = strlen(town);
  int country_len = strlen(country);
  int api_key_len = strlen(api_key);
  int total_len = format_len + endpoint_len + town_len + country_len + api_key_len;

  char *output = (char *)malloc(total_len * sizeof(char) + 1);

  snprintf(output, total_len, format, endpoint, town, country, api_key);

  return (const char *)output;
}

void getData()
{
  tft.fillRect(1, 170, 64, 20, TFT_BLACK);
  tft.fillRect(1, 210, 64, 20, TFT_BLACK);

  // Check the current connection status
  if ((WiFi.status() == WL_CONNECTED)) {
    // Query the weather data
    HTTPClient http;
    http.begin(url); 
    int httpCode = http.GET();  // Make the request

    // Check for the returning code
    if (httpCode > 0)
    { 
      payload = http.getString();
      // Serial.println(httpCode);
      Serial.println(payload);
    } else {
      Serial.println("Error on HTTP request");
    }

    http.end(); // Free the resources
  }
  char inp[1000];
  payload.toCharArray(inp, 1000);
  deserializeJson(doc, inp);

  String tmp2 = doc["main"]["temp"];
  String hum2 = doc["main"]["humidity"];
  String town2 = doc["name"];
  tmp = tmp2;
  hum = hum2;

  Serial.println("Temperature" + String(tmp));
  Serial.println("Humidity" + hum);
  Serial.println(town);
}

void setup(void)
{
  Serial.begin(115200);
  Serial.println("setup starting...");

  // pinMode(PIN_BUTTON_1, INPUT_PULLUP);
  // pinMode(PIN_BUTTON_2, INPUT_PULLUP);

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);

  ledcSetup(pwmLedChannelTFT, pwmFreq, pwmResolution);
  ledcAttachPin(PIN_LCD_BL, pwmLedChannelTFT);
  ledcWrite(pwmLedChannelTFT, backlight[b]);

  tft.print("Connecting to ");
  tft.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(300);
    tft.print(".");
  }

  tft.println("");
  tft.println("WiFi connected.");
  tft.println("IP address: ");
  tft.println(WiFi.localIP());

  // Set the url for getting weather data from
  url = get_endpoint(endpoint, town, country, api_key);

  delay(3000);
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);

  tft.setCursor(2, 232, 1);
  tft.println(WiFi.localIP());
  tft.setCursor(80, 204, 1);
  tft.println("BRIGHT:");

  tft.setCursor(80, 152, 2);
  tft.println("SEC:");
  tft.setTextColor(TFT_WHITE, lightblue);
  tft.setCursor(4, 152, 2);
  tft.println("TEMP:");

  tft.setCursor(4, 192, 2);
  tft.println("HUM: ");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.setFreeFont(&Orbitron_Medium_20);
  tft.setCursor(6, 82);
  tft.println(town);

  tft.fillRect(68, 152, 1, 74, TFT_GREY);

  for (int i = 0; i < b + 1; i++) {
    tft.fillRect(78 + (i * 7), 216, 3, 10, blue);
  }

  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT 0 = 0
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // Initialize a NTPClient to get time
  timeClient.begin();
  timeClient.setTimeOffset(HOUR * GMT_OFFSET);

  getData();
  delay(500);
}

int i = 0;
String tt = "";
int count = 0;
bool inv = 1;
int press1 = 0;
int press2 = 0;

int frame = 0;
String curSeconds = "";

#define COL_1 0
#define MAX_PORTRAIT_WIDTH 170

void loop()
{  
  tft.pushImage(COL_1, 88, MAX_PORTRAIT_WIDTH, 60, ani[frame]);

  frame = (frame + 1) % 10;

  if (digitalRead(35) == 0) {
    if (press2 == 0) {
      press2 = 1;
      tft.fillRect(78, 216, 44, 12, TFT_BLACK);

      b = (b + 1) % 5;

      for (int i = 0; i < b + 1; i++) {
        tft.fillRect(78 + (i * 7), 216, 3, 10, blue);
      }
        
      ledcWrite(pwmLedChannelTFT, backlight[b]);
    }
  } else {
    press2 = 0;
  }

  if (digitalRead(0) == 0) {
    if (press1 == 0) {
      press1 = 1;
      inv = !inv;
      tft.invertDisplay(inv);
    }
  } else {
    press1 = 0;
  }

  if (count == 0) {
    getData();
  }

  count = (count + 1) % 2000;

  tft.setFreeFont(&Orbitron_Medium_20);
  tft.setCursor(2, 187);
  tft.println(tmp.substring(0, 3));

  tft.setCursor(2, 227);
  tft.println(hum + "%");

  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.setTextFont(2);
  tft.setCursor(6, 44);
  tft.println(dayStamp);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }

  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  formattedDate = timeClient.getFormattedDate();
  Serial.println(formattedDate);

  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);

  timeStamp = formattedDate.substring(splitT + 1, formattedDate.length() - 1);

  if (curSeconds != timeStamp.substring(6, 8)) {
    tft.fillRect(78, 170, 48, 28, darkred);
    tft.setFreeFont(&Orbitron_Light_24);
    tft.setCursor(81, 192);
    tft.println(timeStamp.substring(6, 8));
    curSeconds = timeStamp.substring(6, 8);
  }

  tft.setFreeFont(&Orbitron_Light_32);
  String current = timeStamp.substring(0, 5);
  
  if (current != tt) {
    tft.fillRect(3, 8, 120, 30, TFT_BLACK);
    tft.setCursor(5, 34);
    tft.println(timeStamp.substring(0, 5));
    tt = timeStamp.substring(0, 5);
  }

  delay(80);
}
