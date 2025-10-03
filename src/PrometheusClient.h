#pragma once

#include <Arduino.h>
#include <WiFiUdp.h>
#include <NTPClient.h>  // for NTP updates
#include <WiFiClient.h> // for HTTP requests
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>


#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF


class PrometheusClient
{
public:
    PrometheusClient();
    uint16_t *init(int w, int h);
    int refresh();
    bool getTimeseries(int range);

    String getMetric();
    void setMetric(char *metric_p);
     void setTitle(char *title_p);
    void setHost(char *host, int port);
    int getWidth();
    int getHeight();
    static void updateTime(); // call this in loop() once
    String getTime();
    static void beginNTP();



    struct Record
    {
        long timestamp;
        float value;
    };

    static const int MAX_RECORDS = 1000;
    Record records[MAX_RECORDS]; // each instance has its own buffer
private:
    void clearBuffer();

    uint16_t *buffer = nullptr;
    int width = 0;
    int height = 0;
    int refreshCount = 0;
    char *metric = "";
    char *title = "";
    char *prometheusHost = "192.168.2.8";
    int prometheusPort = 9090;

    static WiFiUDP *ntpUDP;
    static NTPClient *timeClient;
    static bool ntpStarted;
    int get_data_range(Record *arr, int range, int step);
    DynamicJsonDocument performPrometheusQuery(const char *metric, unsigned long windowSeconds, unsigned long step);
    bool drawOnBuffer(uint16_t *buffer, int16_t w, int16_t h);

    // Nested helper class, only for PrometheusClient
    class BufferCanvas : public Adafruit_GFX
    {
    public:
        BufferCanvas(uint16_t *buf, int16_t w, int16_t h)
            : Adafruit_GFX(w, h), buf(buf) {}

        void drawPixel(int16_t x, int16_t y, uint16_t color)
        {
            if (x < 0 || y < 0 || x >= _width || y >= _height)
                return;
            uint16_t *buf16 = reinterpret_cast<uint16_t *>(buf);
            buf16[y * _width + x] = color;
        }

    private:
        uint16_t *buf;
    };
};
