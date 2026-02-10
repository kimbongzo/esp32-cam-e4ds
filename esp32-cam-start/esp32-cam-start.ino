// digiclock.ino

#include "tft_setup.h"
#include "stdarg.h"
#include "WiFi.h"
#include "TFT_eSPI.h"
#include "TFT_eWidget.h"
#include "HTTPClient.h"
#include "ArduinoJson.h"
#include "TimeLib.h"
#include "datestr.h"

#define WIFI_SSID "carelab"
#define WIFI_PASSPHRASE "12345678"
#define URL "http://worldtimeapi.org/api/ip"

StaticJsonDocument<2048> doc;
uint16_t cal[5] = { 243, 3669, 216, 3553, 7 };
char timeStr[20];
char dateStr[40];

TFT_eSPI tft = TFT_eSPI();
ButtonWidget btn1 = ButtonWidget(&tft);
ButtonWidget btn2 = ButtonWidget(&tft);
ButtonWidget* btns[] = { &btn1, &btn2 };

bool is24h = true;
bool isDay = true;

void btn1_pressed(void) {
  if (btn1.justPressed()) {
    is24h = !btn1.getState();
    btn1.drawSmoothButton(is24h, 1, TFT_DARKGREY, is24h ? "24h" : "12h");
  }
}

void btn2_pressed(void) {
  if (btn2.justPressed()) {
    isDay = !btn2.getState();
    btn2.drawSmoothButton(isDay, 1, TFT_DARKGREY, isDay ? "day" : "night");
  }
}

void initButtons() {
  uint16_t w = 100;
  uint16_t h = 50;
  uint16_t y = tft.height() - h + 12;
  uint16_t x = tft.width() / 2;
  tft.setTextFont(4);

  btn1.initButtonUL(x - w - 10, y, w, h, TFT_DARKGREY, TFT_BLACK, TFT_DARKGREY, "24h", 1);
  btn1.setPressAction(btn1_pressed);
  btn1.drawSmoothButton(is24h, 1, TFT_BLACK);

  btn2.initButtonUL(x + 10, y, w, h, TFT_DARKGREY, TFT_BLACK, TFT_DARKGREY, "day", 1);
  btn2.setPressAction(btn2_pressed);
  btn2.drawSmoothButton(isDay, 1, TFT_BLACK);
}

void handleButtons() {
  tft.setTextFont(4);
  uint8_t nBtns = sizeof(btns) / sizeof(btns[0]);
  uint16_t x = 0, y = 0;
  bool touched = tft.getTouch(&x, &y);
  for (uint8_t b = 0; b < nBtns; b++) {
    if (touched) {
      if (btns[b]->contains(x, y)) {
        btns[b]->press(true);
        btns[b]->pressAction();
      }
    } else {
      btns[b]->press(false);
      btns[b]->releaseAction();
    }
  }
}

bool shouldSyncTime() {
  time_t t = now();
  bool wifi_on = WiFi.status() == WL_CONNECTED;
  bool should_sync = (minute(t) == 0 && second(t) == 3) || (year(t) == 1970);
  return wifi_on && should_sync;
}

void syncTime() {
  delay(1000);
  HTTPClient http;
  http.begin(URL);
  if (http.GET() > 0) {
    String json = http.getString();
    auto error = deserializeJson(doc, json);
    if (!error) {
      int Y, M, D, h, m, s, ms, tzh, tzm;
      sscanf(doc["datetime"], "%d-%d-%dT%d:%d:%d.%d+%d:%d",
             &Y, &M, &D, &h, &m, &s, &ms, &tzh, &tzm);
      setTime(h, m, s, D, M, Y);
    }
  }
  http.end();
}

void updateDisplay() {
  time_t t = now();
  sprintf(timeStr, " %2d:%02d ",
          (is24h ? hour(t) : hourFormat12(t)), minute(t));
  sprintf(dateStr, "    %s, %s %d, %d    ",
          DAYSTR[weekday(t)], MONTHSTR[month(t)], day(t), year(t));


  uint16_t color = isDay ? TFT_WHITE : TFT_DARKGREY;
  tft.setTextColor(color, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);

  tft.setTextSize(3);
  tft.drawString(timeStr, tft.width() / 2, 120, 7);

  tft.setTextSize(1);
  tft.drawString(dateStr, tft.width() / 2, 230, 4);
}

void setup(void) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSPHRASE);
  while (WiFi.status() != WL_CONNECTED)
    delay(500);

  tft.init();
  tft.setTouch(cal);
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(1);

  initButtons();
}

void loop() {
  if (shouldSyncTime())
    syncTime();
  updateDisplay();
  handleButtons();
  delay(50);
}