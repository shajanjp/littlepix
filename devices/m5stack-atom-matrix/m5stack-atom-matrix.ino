#include <M5Atom.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "time.h"

const char *ssid = "BlackAnt";
const char *password = "B31nth3l1n3";
const char* ntpServer = "pool.ntp.org";

// IST (UTC+5:30)
const long gmtOffset_sec = 19800;
const int daylightOffset_sec = 0;
const uint8_t digits[10][5] = {
  {0b111,0b101,0b101,0b101,0b111}, // 0
  {0b010,0b110,0b010,0b010,0b111}, // 1
  {0b111,0b001,0b111,0b100,0b111}, // 2
  {0b111,0b001,0b111,0b001,0b111}, // 3
  {0b101,0b101,0b111,0b001,0b001}, // 4
  {0b111,0b100,0b111,0b001,0b111}, // 5
  {0b111,0b100,0b111,0b101,0b111}, // 6
  {0b111,0b001,0b010,0b010,0b010}, // 7
  {0b111,0b101,0b111,0b101,0b111}, // 8
  {0b111,0b101,0b111,0b001,0b111}  // 9
};

const char *apiUrl = "https://littlepix.sj.deno.net/api/art?limit=20&size=5";

int spiralOrder[25] = {
    0, 1, 2, 3, 4,
    9, 14, 19, 24,
    23, 22, 21, 20,
    15, 10, 5,
    6, 7, 8,
    13, 18,
    17, 16,
    11,
    12};

// Hardcoded rainbow gradient (Violet → Red)
uint32_t rainbowColors[25] = {
0x3526FF,
0x6627FB,
0xC01ED4,
0xEC1254,
0xF4122A,
0x3046FB,
0x713BF8,
0xC223D8,
0xE92360,
0xF53525,
0x17B8EF,
0x419CEA,
0x9583C6,
0xE68843,
0xF78515,
0x12E5D2,
0x29E4B5,
0x98EC55,
0xEEEA0D,
0xF8BE12,
0x2AF96C,
0x40FA57,
0xADFC1E,
0xF3FB08,
0xF3EF08
};

void setup()
{
  M5.begin(true, false, true);
  Serial.begin(115200);

  displayLoader();

  connectWiFi();

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  displayTime();
}

void loop()
{
  fetchAndDisplayArts();

  // After finishing all, wait 30 sec before refetching
  delay(10000);
}

void connectWiFi()
{
  WiFi.begin(ssid, password);
  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED)
  {
    displayLoader();
    Serial.print(".");
  }

  Serial.println("\nConnected!");
}

void displayLoader()
{
  M5.dis.clear();

  for (int i = 0; i < 25; i++)
  {
    M5.dis.drawpix(spiralOrder[i], rainbowColors[spiralOrder[i]]);
    delay(50);
  }
  delay(500);
}

void fetchAndDisplayArts()
{

  if (WiFi.status() != WL_CONNECTED)
    return;

  HTTPClient http;
  http.begin(apiUrl);

  int httpCode = http.GET();

  if (httpCode == 200)
  {

    String payload = http.getString();
    Serial.println("Received data");

    StaticJsonDocument<8192> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error)
    {
      Serial.println("JSON parse failed");
      return;
    }

    JsonArray dataArray = doc["data"];

    for (JsonObject art : dataArray)
    {

      const char *name = art["name"];
      const char *author = art["author"];

      Serial.print("Displaying: ");
      Serial.print(name);
      Serial.print(" by ");
      Serial.println(author);

      JsonObject mapping = art["mapping"];

      drawMapping(mapping);

      delay(5000); // 🔥 10 second delay
    }
  }

  http.end();
}

void drawMapping(JsonObject mapping)
{

  M5.dis.clear(); // Clear previous frame

  for (JsonPair kv : mapping)
  {

    int index = atoi(kv.key().c_str());
    const char *hexColor = kv.value();

    if (hexColor[0] == '#' && strlen(hexColor) == 7)
    {

      uint32_t color = strtol(hexColor + 1, NULL, 16);
      M5.dis.drawpix(index, color);
    }
  }
}

void drawDigit(int digit, uint32_t color) {
  M5.dis.clear();

  for (int row = 0; row < 5; row++) {
    for (int col = 0; col < 3; col++) {
      if (digits[digit][row] & (1 << (2 - col))) {
        int index = row * 5 + col + 1; // centered horizontally
        M5.dis.drawpix(index, color);
      }
    }
  }
}

void displayTime() {

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return;
  }

  int hour = timeinfo.tm_hour;
  int minute = timeinfo.tm_min;

  int hTens = hour / 10;
  int hUnits = hour % 10;
  int mTens = minute / 10;
  int mUnits = minute % 10;

  // Show 4 screens
  drawDigit(hTens, 0x00FF00);   // Red
  delay(2000);

  drawDigit(hUnits, 0x00FF00);  // Green
  delay(2000);

  M5.dis.clear();
  M5.dis.drawpix(7, 0xFFFF00);
  M5.dis.drawpix(17, 0xFFFF00);
  delay(2000);

  drawDigit(mTens, 0x0000FF);   // Blue
  delay(2000);

  drawDigit(mUnits, 0x0000FF);  // Yellow
  delay(2000);
}