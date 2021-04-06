//load libraries - ESP8266 Wifi, SSL support, JSON parser, Time Libraries, TFT driver, SPIFFS and SPI driver, JPEG decoder, and Adafruit Graphics library for epaper display
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include <Time.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include <FS.h>
#include <JPEGDecoder.h>
#include "gill28pt7b.h" //Font file, keep in same folder as the .ino
#include <coredecls.h>

TFT_eSPI tft = TFT_eSPI();

//Put your SSID and PW here
const char* ssid = "YOUR_SSID"; //enter your SSID
const char* password = "YOUR_PASSWORD"; //enter your wifi password

int hour_c;
int min_c;
int sec_c;

void time_is_set() {                                        // Report time was pulled from NTP
  Serial.println("********* NTP Server Timestap Synchronisation  *********");
}
uint32_t sntp_update_delay_MS_rfc_not_less_than_15000 () {
  return 6 * 60 * 60 * 1000UL; // Grab NTP time every 6h
}
char* localTime() {
  struct tm tm;
  static char buf2[26];
  time_t now = time(&now);
  localtime_r(&now, &tm);
  strftime (buf2, sizeof(buf2), R"(["%T","%d.%m.%Y"])", &tm);
  return buf2;
}

const char* host = "XXXXX.herokuapp.com"; //enter your nightscout host here - no "https://" needed
const int httpsPort = 443;
const size_t bufferSize = 3 * JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + 2 * JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(13) + 280;
// SHA1 fingerprint of the host's SSL certificate
const char* fingerprint = "00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00"; //enter the https fingerprint if you want this verification

// You can edit these values if you want. They change at which point the BG number changes color.
int HighBG = 180;
int LowBG = 80;
int CritBG = 60;
#define FONT &gill28pt7b
#define GFXFF 1

void setup() {

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  //Print something during boot so you know it's doing something

  tft.setTextColor(TFT_YELLOW, TFT_BLACK); // Note: the new fonts do not draw the background colour
  tft.setCursor(80, 80);
  tft.println("Starting up...");
  Serial.begin(115200);
  Serial.setTimeout(2000);
  Serial.println();
  Serial.println("Starting up...");
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS initialisation failed!");
    tft.setCursor(80, 100);
    tft.println("SPIFFS init failed");
    while (1) yield(); // Stay here twiddling thumbs waiting
  }
  Serial.println("\r\nSPIFFS available!");

  // ESP32 will crash if any of the fonts or graphics are missing
  bool arrows_missing = false;
  if (SPIFFS.exists("/Flat.jpg")    == false) arrows_missing = true;

  if (arrows_missing)
  {
    Serial.println("\r\nArrow images missing in SPIFFS, did you upload them?");
    tft.setCursor(80, 100);
    tft.println("Files missing!");
    while (1) yield();
  }
  else Serial.println("\r\Images found OK.");
  WiFi.hostname("Nightscout_Display");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  configTime("CET-1CEST,M3.5.0/02,M10.5.0/03", "de.pool.ntp.org", "192.168.0.1"); //change this to your time servers of choice. Pre-set for Berlin timezone and automatic summer/wintertime switching. Documentation on how to adjust: https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
  settimeofday_cb(time_is_set);

}
void loop() {
  tm tm;
  char buf[20];
  constexpr uint32_t INTERVAL {1000};
  static uint32_t previousMillis;
  if (millis() - previousMillis >= INTERVAL) {
    previousMillis += INTERVAL;
    time_t now = time(nullptr);
    localtime_r(&now, &tm);
    strftime (buf, sizeof(buf), "%d.%m.%Y %T", &tm);
    //Serial.println(buf); //Print current date and time
    sec_c = (tm.tm_sec);

  }
  if ((sec_c) == 0) { //check only on the full minute. Side effect is that it can take up to a minute for the "Starting up..." screen to disappear.
    //Serial.println(WiFi.localIP());
    WiFiClientSecure client;
    client.setInsecure();

    if (!client.connect(host, httpsPort)) {
      Serial.println("connection failed");
      Serial.println("Sleeping...");
      delay(30000);
      return;
    }
    if (client.verify(fingerprint, host)) {
      Serial.println("certificate matches");

    } else {
      Serial.println("certificate doesn't match");

    }

    String url = "/pebble";
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "User-Agent: BuildFailureDetectorESP8266\r\n" +
                 "Connection: close\r\n\r\n");

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        //Serial.println("headers received");
        break;
      }
    }
    String line2 = client.readStringUntil('\n');
    //read the JSON line into "line2"
    line2 = client.readStringUntil('\n');
    //parse the JSON into variables
    DynamicJsonBuffer jsonBuffer(bufferSize);
    JsonObject& root = jsonBuffer.parseObject(line2);
    String status0_now = root["status"][0]["now"];
    status0_now = status0_now.substring(0, status0_now.length() - 3);
    time_t status0_now1 = status0_now.toInt();
    JsonObject& bgs0 = root["bgs"][0];
    int bgs0_sgv = bgs0["sgv"];
    int bgs0_trend = bgs0["trend"];
    int bgs0_delta = bgs0["bgdelta"];
    const char* bgs0_direction = bgs0["direction"];
    time_t bgs0_datetime = bgs0["datetime"];
    String bgs0_datetime2 = bgs0["datetime"];
    bgs0_datetime2 = bgs0_datetime2.substring(0, bgs0_datetime2.length() - 3);
    time_t bgs0_datetime3 = bgs0_datetime2.toInt();
    JsonObject& cals0 = root["cals"][0];
    float cals0_slope = cals0["slope"];
    float cals0_intercept = cals0["intercept"];
    int cals0_scale = cals0["scale"];
    int BG = (bgs0_sgv);
    String delta = "0";
    String positive = "+";
    if  ((bgs0_delta) >= 0) {
      delta = positive + bgs0_delta;
    }
    else {
      delta = (bgs0_delta);
    }

    int dataAge = (status0_now1 - bgs0_datetime3) / 60;
    char* directarr = "";
    if (String(bgs0_direction) == "Flat") {
      directarr = "→";
    }  else if (String(bgs0_direction) == "FortyFiveUp") {
      directarr = "↗";
    }  else if (String(bgs0_direction) == "FortyFiveDown") {
      directarr = "↘";
    } else if (String(bgs0_direction) == "DoubleUp") {
      directarr = "↑↑";
    } else if (String(bgs0_direction) == "DoubleDown") {
      directarr = "↓↓";
    }  else if (String(bgs0_direction) == "SingleUp") {
      directarr = "↑";
    }  else if (String(bgs0_direction) == "SingleDown") {
      directarr = "↓";
    }
    
    int bgcol = TFT_WHITE;
    int agecol = TFT_WHITE;
    int agepos = 38;

    //Change BG number font color according to high, low or critical low numbers
    if (BG >= HighBG) {
      bgcol = TFT_ORANGE;
    }
    else if ((BG <= LowBG) && (BG > CritBG)) {
      bgcol = TFT_YELLOW;
    }
    else if (BG <= CritBG) {
      bgcol = TFT_RED;
    }
    else {
      bgcol = TFT_WHITE;
    }

    //Turn data age string red if data is older than 15 mins, display age data
    if (dataAge >= 15) {
      agecol = TFT_RED;
    }
    else {
      agecol = TFT_WHITE;
    }
    if (dataAge >= 10) {
      agepos = 30;
    }
    if (dataAge >= 100) {
      agepos = 23;
    }

    tft.fillRect (0, 0, 240, 48, TFT_BLACK); //clearing date string for less flicker on refresh
    tft.setFreeFont(&FreeSerifBold9pt7b);
    tft.setTextColor((agecol), TFT_BLACK);
    tft.setCursor(agepos, 20);
    if ((dataAge == 0) && (bgs0_sgv == 0)) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.print("Data download error!");
    }
    else {
      tft.print("Last Data: ");
      tft.print(dataAge);
      if (dataAge == 1) {
        tft.println(" min ago");
      }
      else tft.println(" mins ago");
    }
    //show clock.
    tft.fillRect (60, 40, 120, 55, TFT_BLACK); //clearing time string for less flicker on refresh
    tft.setFreeFont(&FreeSerifBold24pt7b);
    tft.setTextColor(TFT_BLUE, TFT_BLACK);
    tft.setCursor(65, 80);
    hour_c = (tm.tm_hour);
    min_c  = (tm.tm_min);

    //Insert leading zero if single digit and print to serial and TFT
    Serial.println("Current Time:");
    if ((hour_c) < 10) Serial.print("0");
    Serial.print(hour_c);
    Serial.print(":");
    if ((min_c) < 10) Serial.print("0");
    Serial.print(min_c);
    Serial.println();

    if ((hour_c) < 10) tft.print("0");
    tft.print(hour_c);
    tft.print(":");
    if ((min_c) < 10) tft.print("0");
    tft.print(min_c);

    //Display BG number and arrow jpg, position starting point of number 40 pixels to the right if only 2 digits
    int digits;
    if ((bgs0_sgv) < 100) {
      digits = 70;
    }
    else {
      digits = 30;
    }
    tft.fillRect (30, 110, 148, 62, TFT_BLACK); //clearing BG string for less flicker on refresh, still flickers some
    tft.setCursor(digits, 159);
    tft.setFreeFont(FONT);
    tft.setTextColor((bgcol), TFT_BLACK);
    tft.println(bgs0_sgv);

    //show Delta
    int deltapos = 54;
    if (bgs0_delta >= 10) {
      deltapos = 42;
    }
    tft.fillRect (35, 180, 200, 60, TFT_BLACK); //clearing Delta string for less flicker on refresh
    tft.setFreeFont(&FreeSerifBold18pt7b);
    tft.setCursor(deltapos, 220);
    tft.setTextColor(TFT_BLUE, TFT_BLACK);
    tft.print("Delta: ");
    tft.println(delta);
    //Check trend number from json and show the matching arrow
    if ((bgs0_trend) == 1) {
      drawJpeg("/DoubleUp.jpg", 180, 116);
    }
    if ((bgs0_trend) == 2) {
      drawJpeg("/Up.jpg", 180, 116);
    }
    if ((bgs0_trend) == 3) {
      drawJpeg("/FortyFiveUp.jpg", 180, 116);
    }
    if ((bgs0_trend) == 4) {
      drawJpeg("/Flat.jpg", 180, 116);
    }
    if ((bgs0_trend) == 5) {
      drawJpeg("/FortyFiveDown.jpg", 180, 116);
    }
    if ((bgs0_trend) == 6) {
      drawJpeg("/Down.jpg", 180, 116);
    }
    if ((bgs0_trend) == 7) {
      drawJpeg("/DoubleDown.jpg", 180, 116);
    }
    if ((bgs0_trend) == 8) {
      tft.fillRect (175, 114, 60, 50, TFT_BLACK);
    }

  }
}
