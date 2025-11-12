#include <WiFi.h>

#include "M5AtomS3.h"
#include <ArduinoGraphics.h>

#include <PrometheusClient.h>

// WiFi credentials
const char* ssid = "< YOUR SSID >";
const char* password = "< YOUR PASSWORD >";



int w1 = 118;
int h1 = 118;

uint16_t* myBuffer;  // buffer from c1

// Prometheus settings
char* prometheushost = "192.168.2.8";  // My Raspberry IP
int prometheusport = 9090;

WiFiClient client;

PrometheusClient c1;
Image img;  // Buffer hosting the image result from Library

LovyanGFX* gfx;

void setup() {
  auto cfg = M5.config();
  AtomS3.begin(cfg);
  Serial.begin(115200);
  gfx = &AtomS3.Display;
  AtomS3.Display.setSwapBytes(true);
  AtomS3.Display.setRotation(1);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  myBuffer = c1.init(w1, h1);
  img = Image(ENCODING_RGB16, (uint8_t*)myBuffer, w1, h1);
  c1.setMetric("100*avg(rate(node_cpu_seconds_total{mode='user'}[1m]))");
  c1.setTitle("Cpu util - %");
  c1.setThr1(50);
  c1.setThr2(75);
  c1.enableThr(true);
  c1.setHost( prometheushost, prometheusport);
  PrometheusClient::beginNTP();
  c1.updateTime();
}


void loop() {
  Serial.print("Free heap before refresh: ");
  Serial.println(ESP.getFreeHeap());

  c1.getTimeseries(3600);
  gfx->pushImage(5, 5, w1, h1, myBuffer);
  Serial.print("Free heap after refresh: ");
  Serial.println(ESP.getFreeHeap());
  Serial.println(c1.getTime());
  delay(5000);
}
