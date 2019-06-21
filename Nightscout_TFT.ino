//load libraries - ESP8266 Wifi, SSL support, JSON parser, Time Libraries, and Adafruit Graphics library for epaper display
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include <Time.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include <FS.h>
#include <JPEGDecoder.h>

//init setup for the epaper display
TFT_eSPI tft = TFT_eSPI();

//Put your SSID and PW here
const char* ssid = "SSID"; //enter your SSID
const char* password = "PASSWORD"; //enter your wifi password

//Change time offset from UTC, 1 hour = 3600
int TimeOffset = 7200;



const char* host = "XXXXX.herokuapp.com"; //enter your nightscout host here - no "https://" needed
const int httpsPort = 443;
const size_t bufferSize = 3 * JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + 2 * JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(13) + 280;
// SHA1 fingerprint of the host's SSL certificate
const char* fingerprint = "00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00"; //enter the https fingerprint if you want this verification

// You can edit these values if you want. They change at which point the BG number changes color.
int HighBG = 180;
int LowBG = 80;
int CritBG = 60;
String numfont = "gill";

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

  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS initialisation failed!");
    while (1) yield(); // Stay here twiddling thumbs waiting
  }
  Serial.println("\r\nSPIFFS available!");

  // ESP32 will crash if any of the fonts are missing
  bool font_missing = false;
  if (SPIFFS.exists("/Flat.jpg")    == false) font_missing = true;
  if (SPIFFS.exists("/gill.vlw")    == false) font_missing = true;

  if (font_missing)
  {
    Serial.println("\r\nFont or arrow images missing in SPIFFS, did you upload them?");
    while (1) yield();
  }
  else Serial.println("\r\nFonts and images found OK.");




}
void loop() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("");

  Serial.println(WiFi.localIP());

  // Use WiFiClientSecure class to create TLS connection
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

  //Serial.println("request sent");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      //Serial.println("headers received");
      break;
    }
  }
  String line2 = client.readStringUntil('\n');
  Serial.println("==========");
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
  status0_now1 = status0_now1 + (TimeOffset);
  bgs0_datetime3 = bgs0_datetime3 + (TimeOffset);
  int BG = (bgs0_sgv);
  String delta = "0";
  String positive = "+";
  if  ((bgs0_delta) >= 0) {
    delta = positive + bgs0_delta;
  }
  else {
    delta = (bgs0_delta);
  }
  Serial.print("Time Now: ");
  Serial.print(hour(status0_now1));
  Serial.print(":");
  Serial.println(minute(status0_now1));
  String timeNow = String(hour(status0_now1)) + ":";
  if (minute(status0_now1) < 10) {
    timeNow = timeNow + "0";
  }
  timeNow = timeNow + String(minute(status0_now1));
  //print current time
  Serial.print("As at: ");
  Serial.print(hour(bgs0_datetime3));
  Serial.print(":");
  Serial.println(minute(bgs0_datetime3));
  int dataAge = (status0_now1 - bgs0_datetime3) / 60;
  Serial.print("Data Age (minutes): ");
  Serial.println(dataAge);
  Serial.print("Current Delta: ");
  Serial.println(delta);
  Serial.print("Blood Glucose is:  ");
  Serial.print(String(bgs0_sgv) + " ");
  char* directarr = "";
  if (String(bgs0_direction) == "Flat") {
    directarr = "→︎";
  }  else if (String(bgs0_direction) == "FortyFiveUp") {
    directarr = "↗︎";
  }  else if (String(bgs0_direction) == "FortyFiveDown") {
    directarr = "↘";
  } else if (String(bgs0_direction) == "DoubleUp") {
    directarr = "↑↑";
  } else if (String(bgs0_direction) == "DoubleDown") {
    directarr = "↓↓";
  }
  Serial.println(directarr);
  Serial.println(bgs0_trend);
  tft.fillScreen(TFT_BLACK);
  int bgcol = TFT_WHITE;
  int agecol = TFT_GREEN;


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
    agecol = TFT_GREEN;
  }

  tft.setFreeFont(&FreeSerif9pt7b);
  tft.setTextColor((agecol), TFT_BLACK);
  tft.setCursor(45, 20);
  tft.print("Last Data: ");
  tft.print(dataAge);
  if (dataAge == 1) {
    tft.println(" min ago");
  }
  else tft.println(" mins ago");


  //show clock. Uses the time from NS server, not too accurate.
  tft.setFreeFont(&FreeSerifBold24pt7b);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.setCursor(72, 80);
  String timeCur = String(hour(status0_now1)) + ":";
  if (minute(status0_now1) < 10) {
    timeCur = timeCur + "0";
  }
  if (hour(status0_now1) < 10) {
    timeCur = "0" + timeCur;
  }
  timeCur = timeCur + String(minute(status0_now1));
  tft.print(timeCur);

  //Load number font and display BG number and arrow jpg, position starting point of number 40 pixels to the right if only 2 digits
  int digits = 40;
  if ((bgs0_sgv) < 100) {
    digits = 80;
  }
  else {
    digits = 40;
  }
  tft.setCursor(digits, 122);
  tft.loadFont(numfont);
  tft.setTextColor((bgcol), TFT_BLACK);
  tft.println(String(bgs0_sgv) + " ");
  tft.unloadFont();
  //show Delta
  tft.setFreeFont(&FreeMonoBoldOblique18pt7b);
  tft.setCursor(25, 220);
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

  //Pause for 45 secs to avoid spamming the server, but keep the clock semi-accurate
  Serial.println("Pausing for 45 seconds");
  delay(45000);
}



//Display BG Number
