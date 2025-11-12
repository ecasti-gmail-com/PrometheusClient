#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <M5Unified.h>
#include <M5GFX.h>
#include <WiFi.h>
#include <PrometheusClient.h>


#define SD_SPI_SCK_PIN  36
#define SD_SPI_MISO_PIN 35
#define SD_SPI_MOSI_PIN 37
#define SD_SPI_CS_PIN   4


// WiFi credentials from SD
String ssid;
String password;

// Prometheus settings
String prometheushost;
int prometheusport;
int enablehttps = 0;

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
 // M5.Display.setRotation(3); // not on CoreS3
  M5.Display.setSwapBytes(true);
  M5.Display.drawPngFile(SD, "/loading_mini.png", 50, 25);
  read_config();
  w = M5.Lcd.width();
  h = M5.Lcd.height();

  Serial.begin(115200);
  WiFi.begin(ssid, password);
  M5.Display.drawPngFile(SD, "/connecting_mini.png", 50, 25);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  M5.Display.drawPngFile(SD, "/connected_mini.png", 50, 25);
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
  Serial.println("Setup Done");
}


void refresh_image() {
  for (int i = 0; i < 4; i++) {
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
  if ( ! clean) {
     M5.Lcd.fillScreen(WHITE);
     clean = true;
  }
  for (int i = 0; i < 4; i++) {
    M5.Display.pushImage(panel[i].x, panel[i].y, panel[i].w, panel[i].h, panel[i].myBuffer);
  }
}

void draw_header() {
  M5.Display.fillRect(0, 0, 320, 25, 0xFFFF);
  M5.Display.setTextColor(0x0000, 0xFFFF);
  M5.Display.setFont(&fonts::FreeMonoBold12pt7b);
  M5.Display.drawString(panel[0].client->getTime() + " GMT", 20, 2);
  M5.Display.drawRect(290, 5, 20, 10, 0x0000);
  M5.Display.drawRect(291, 6, 18, 8,  0x0000);
  M5.Display.fillRect(310, 7, 3, 6, 0x0000);
  int charging = M5.Power.isCharging();
  int level = M5.Power.getBatteryLevel();
  int bar = int((level * 16) / 100);

  if (level > 45) {
    M5.Display.fillRect(292, 7, bar, 6, 0x07E0);
  } else if (level > 20) {
    M5.Display.fillRect(292, 7, bar, 6, 0xFC40);
  } else {
    M5.Display.fillRect(292, 7, bar, 6, 0xF800);
  }

  if (charging > 0) {
    M5.Display.fillTriangle(317, 6, 315, 11, 317, 11, 0xFC40);
    M5.Display.fillTriangle(317, 14, 319, 9, 317, 9, 0xFC40);
  }
  if (WiFi.status() == WL_CONNECTED) {
    int rssi = WiFi.RSSI();
    M5.Display.drawTriangle(278, 14, 288, 5, 288, 14, 0x0000);
    int wbar = (int)(((rssi + 100) * 8) / 100);
    M5.Display.fillTriangle(280, 13, 280 + wbar, 13, 280 + wbar, 13 - wbar, 0x07E0);
  }


}

void refresh_zoom() {
  panel[4].client->getTimeseries(3600);
  M5.Display.pushImage(panel[4].x, panel[4].y, panel[4].w, panel[4].h, panel[4].myBuffer);
}

void drawAlerts() {
  // Temporarely disabled
  /*
  int maxalert = 0;
  for (int i = 0; i < 4; i++) {
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
  */
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
    nx = touchPoint[0].x;
    ny = touchPoint[0].y;
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
      for (int ii = 0; ii < 4; ii++) {
        if ((nx > panel[ii].x) && (ny > panel[ii].y) && (nx < panel[ii].w + panel[ii].x) && (ny < panel[ii].h + panel[ii].y)) {
          panel[4].client->setMetric(panel[ii].client->getMetric());
          panel[4].client->setTitle(panel[ii].client->getTitle());
          panel[4].client->setThr1(panel[ii].client->getThr1());
          panel[4].client->setThr2(panel[ii].client->getThr2());
          panel[4].client->enableThr(panel[ii].client->getEnabledThr());
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