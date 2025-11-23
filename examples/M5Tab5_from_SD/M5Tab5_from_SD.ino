#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <M5Unified.h>
#include <M5GFX.h>
#include <WiFi.h>
#include <PrometheusClient.h>


#define SD_SPI_CS_PIN 42
#define SD_SPI_SCK_PIN 43
#define SD_SPI_MOSI_PIN 44
#define SD_SPI_MISO_PIN 39


// WiFi credentials from SD
String ssid;
String password;

// Prometheus settings
String prometheushost;
int prometheusport;
int enablehttps = 0;
String api_username;
String api_password;
const char* Timezone = "CET-1CEST,M3.5.0,M10.5.0/3";  // Choose your time zone from: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv


int otx = 0;
int oty = 0;
int nx, ny;
bool clean = false;


WiFiClient client;

struct panel_d {
  PrometheusClient* client;
  uint16_t* myBuffer;
  int type = 0;
  String metric;  // safer than char*
  String title;   // safer than char*
  int w = 0;
  int h = 0;
  int x = 0;
  int y = 0;
  int thr1;
  int thr2;
  int timerange;
  bool enableThr;
  bool enableTrend;
};
PrometheusClient clients[10];
struct panel_d panel[10];



m5::touch_point_t touchPoint[5];  //Tab5 supports up to 5-point touch
static bool drawed = false;
static int32_t w;
static int32_t h;

long last_refresh;
int mode = 0;

void setup() {
  M5.begin();
  SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
  if (!SD.begin(SD_SPI_CS_PIN, SPI, 25000000)) {
    Serial.println("\n SD card not detected\n");
    while (1)
      ;
  } else {
    Serial.println("\n SD card detected\n");
  }
  M5.Lcd.fillScreen(WHITE);
  M5.Display.setRotation(3);
  M5.Display.setSwapBytes(true);
  M5.Display.drawPngFile(SD, "/loading.png", 300, 50);
  read_config();
  w = M5.Lcd.width();
  h = M5.Lcd.height();

  Serial.begin(115200);
  WiFi.begin(ssid, password);
  M5.Display.drawPngFile(SD, "/connecting.png", 300, 50);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  M5.Display.drawPngFile(SD, "/connected.png", 300, 50);
  Serial.println("\nWiFi connected");
  Serial.print("Battery: ");
  Serial.println(M5.Power.isCharging());
  Serial.print("Level: ");
  Serial.println(M5.Power.getBatteryLevel());
  M5.Power.setBatteryCharge(true);

  draw_header();

  PrometheusClient::beginNTP();
  panel[0].client->updateTime();
  draw_header();
  last_refresh = millis();
}


void refresh_image() {
  for (int i = 0; i < 9; i++) {
    switch (panel[i].type) {
      case 0:
        panel[i].client->getStat(panel[i].timerange);
        break;
      case 1:
        panel[i].client->getGauge(panel[i].timerange);
        break;
      case 2:
        panel[i].client->getTimeseries(panel[i].timerange);
        break;
    }
  }
  if (!clean) {
    M5.Lcd.fillScreen(WHITE);
    clean = true;
  }
  for (int i = 0; i < 9; i++) {
    M5.Display.pushImage(panel[i].x, panel[i].y, panel[i].w, panel[i].h, panel[i].myBuffer);
  }
}

void draw_header() {
  M5.Display.fillRect(0, 0, 1280, 50, 0xFFFF);
  M5.Display.setTextColor(0x0000, 0xFFFF);
  M5.Display.setFont(&fonts::FreeMonoBold24pt7b);
  M5.Display.drawString(panel[0].client->getTime() + " GMT", 50, 5);
  M5.Display.drawRect(1200, 10, 40, 20, 0x0000);
  M5.Display.drawRect(1201, 11, 38, 18, 0x0000);
  M5.Display.fillRect(1240, 13, 6, 14, 0x0000);
  int charging = M5.Power.isCharging();
  int level = M5.Power.getBatteryLevel();
  int bar = int((level * 36) / 100);

  if (level > 45) {
    M5.Display.fillRect(1202, 12, bar, 16, 0x07E0);
  } else if (level > 20) {
    M5.Display.fillRect(1202, 12, bar, 16, 0xFC40);
  } else {
    M5.Display.fillRect(1202, 12, bar, 16, 0xF800);
  }
  if (charging > 0) {
    M5.Display.fillTriangle(1260, 8, 1250, 24, 1260, 24, 0xFC40);
    M5.Display.fillTriangle(1260, 32, 1270, 16, 1260, 16, 0xFC40);
  }
  if (WiFi.status() == WL_CONNECTED) {
    int rssi = WiFi.RSSI();
    M5.Display.drawTriangle(1170, 30, 1190, 30, 1190, 10, 0x0000);
    M5.Display.drawTriangle(1170, 29, 1189, 29, 1189, 10, 0x0000);
    int wbar = (int)(((rssi + 100) * 15) / 100);
    M5.Display.fillTriangle(1173, 28, 1173 + wbar, 28, 1173 + wbar, 28 - wbar, 0x07E0);
  }
}

void refresh_zoom() {
  panel[9].client->getTimeseries(3600);
  M5.Display.pushImage(panel[9].x, panel[9].y, panel[9].w, panel[9].h, panel[9].myBuffer);
}

void drawAlerts() {
  int maxalert = 0;
  for (int i = 0; i < 9; i++) {
    if (panel[i].client->getAlert() > maxalert) {
      maxalert = panel[i].client->getAlert();
    }
  }
  if (maxalert == 0) {
    M5.Display.drawPngFile(SD, "/normal.png", 920, 360, 350, 350);
    //M5.Display.drawPngFile( "/normal.png");
  } else if (maxalert < 2) {
    M5.Display.drawPngFile(SD, "/warning.png", 920, 360, 350, 350);
    //M5.Display.drawPngFile( "/warning.png");
  } else {
    M5.Display.drawPngFile(SD, "/critical.png", 920, 360, 350, 350);
    //  M5.Display.drawPngFile( "/critical.png");
  }
}

void loop() {
  M5.update();
  int nums = M5.Lcd.getTouchRaw(touchPoint, 5);
  if ((millis() - last_refresh) > 5000) {
    if (mode == 0) {
      refresh_image();
      draw_header();
      drawAlerts();
    } else {
      refresh_zoom();
      draw_header();
    }
    last_refresh = millis();
  }

  if ((nums > 0) && (otx != touchPoint[0].x) && (oty != touchPoint[0].y)) {
    nx = 1280 - touchPoint[0].y;
    ny = touchPoint[0].x;
    Serial.print("Nums = ");
    Serial.println(nums);
    Serial.print("Size = ");
    Serial.println(touchPoint[0].size);
    Serial.print("Id = ");
    Serial.println(touchPoint[0].id);
    Serial.print("X = ");
    Serial.println(nx);
    Serial.print("Y = ");
    Serial.println(ny);
    otx = touchPoint[0].x;
    oty = touchPoint[0].y;

    if (mode == 0) {
      for (int ii = 0; ii < 9; ii++) {
        if ((nx > panel[ii].x) && (ny > panel[ii].y) && (nx < panel[ii].w + panel[ii].x) && (ny < panel[ii].h + panel[ii].y)) {
          panel[9].client->setMetric(panel[ii].client->getMetric());
          panel[9].client->setTitle(panel[ii].client->getTitle());
          panel[9].client->setThr1(panel[ii].client->getThr1());
          panel[9].client->setThr2(panel[ii].client->getThr2());
          panel[9].client->enableThr(panel[ii].client->getEnabledThr());
          Serial.println(panel[ii].client->getMetric());
          mode = 1;
          M5.Lcd.fillScreen(WHITE);
          last_refresh = 0;
        }
      }
    } else {
      mode = 0;
      last_refresh = 0;
      M5.Lcd.fillScreen(WHITE);
    }
    vTaskDelay(1);
  }
  delay(5);
}