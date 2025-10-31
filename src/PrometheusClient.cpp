#include "PrometheusClient.h"

// ✅ Define static members here

WiFiUDP *PrometheusClient::ntpUDP = nullptr;
NTPClient *PrometheusClient::timeClient = nullptr;
bool PrometheusClient::ntpStarted = false;

PrometheusClient::PrometheusClient() {}

void PrometheusClient::beginNTP()
{
  if (!ntpStarted)
  {
    ntpUDP = new WiFiUDP();
    timeClient = new NTPClient(*ntpUDP, "pool.ntp.org", 0, 600000);
    timeClient->begin();
    ntpStarted = true;
  }
}

void PrometheusClient::updateTime()
{
  if (timeClient)
  {
    timeClient->update();
  }
}

String PrometheusClient::getTime()
{
  if (timeClient)
  {
    return timeClient->getFormattedTime();
  }
  return "00:00:00";
}

uint16_t *PrometheusClient::init(int w, int h)
{
  // free previous buffer if needed
  if (buffer)
  {
#if defined(ARDUINO_GIGA)
    if (usingSDRAM)
    {
      SDRAM.free(buffer);
    }
    else
    {
      delete[] buffer;
    }
#elif defined(ESP32)
    if (usingPSRAM)
    {
      free(buffer);
    }
    else
    {
      delete[] buffer;
    }
#else
    delete[] buffer;
#endif
    buffer = nullptr;
  }

  this->width = w;
  this->height = h;
  size_t size = width * height * sizeof(uint16_t);

#if defined(ARDUINO_GIGA)
  buffer = (uint16_t *)SDRAM.malloc(size);
  usingSDRAM = (buffer != nullptr);
  Serial.println(usingSDRAM ? "[SDRAM] Buffer allocated" : "[SDRAM] Allocation failed, using heap");
  if (!usingSDRAM)
    buffer = new uint16_t[width * height];

#elif defined(ESP32)
  buffer = (uint16_t *)ps_malloc(size);
  usingPSRAM = (buffer != nullptr);
  Serial.println(usingPSRAM ? "[PSRAM] Buffer allocated" : "[PSRAM] Allocation failed, using heap");
  if (!usingPSRAM)
    buffer = new uint16_t[width * height];

#else
  buffer = new uint16_t[width * height];
#endif

  clearBuffer();
  return buffer;
}

void PrometheusClient::setMetric(char *metric_p)
{
  this->metric = metric_p;
}

void PrometheusClient::setTitle(char *title_p)
{
  this->title = title_p;
}

void PrometheusClient::setHost(char *host, int port)
{
  this->prometheusHost = host;
  this->prometheusPort = port;
}

char * PrometheusClient::getMetric()
{
  return this->metric;
}

char * PrometheusClient::getTitle()
{
  return this->title;
}

void PrometheusClient::setthr1(long thr)
{
  thr1 = thr;
}

int PrometheusClient::getAlert()
{
  return this->alert;
}


String PrometheusClient::getAlertString()
{
  if (this->alert == 2)
  {
    return "Critical";
  }
  else if (this->alert == 1)
  {
    return "Warning";
  }
  else
  {
    return "Normal";
  }
}

void PrometheusClient::setthr2(long thr)
{
  thr2 = thr;
}

void PrometheusClient::enableThr(bool enabled)
{
  showthr = enabled;
};

bool PrometheusClient::getEnabledThr()
{
  return this->showthr ;
};

void PrometheusClient::enableTrend(bool enabled)
{
  showtrend = enabled;
};

long PrometheusClient::getthr1(){
  return this->thr1;
}
    
long PrometheusClient::getthr2(){
  return this->thr2;
}

int PrometheusClient::getWidth()
{
  return this->width;
}

int PrometheusClient::getHeight()
{
  return this->height;
}

int PrometheusClient::refresh()
{
  refreshCount++;
  // timeClient.update();   // update NTP
  BufferCanvas c(this->buffer, this->width, this->height);
  c.fillScreen(0xFFFF);
  c.drawLine(0, 0, this->width - 1, this->height - 1, 0xF800);
  c.drawLine(0, this->height - 1, this->width - 1, 0, 0xF800);
  c.setCursor(10, 10);
  c.setTextColor(0x0000);
  c.print(metric);
  drawOnBuffer(this->buffer, this->width, this->height);
  return refreshCount;
}

bool PrometheusClient::getGauge(int range)
{
  this->alert = 0;
  float max, min, avg, current_value;
  int outer_radius, inner_radius;
  BufferCanvas c(this->buffer, this->width, this->height);
  c.fillScreen(WHITE); // white background
  c.drawRect(0, 0, this->width , this->height , BLACK);
  c.setCursor(5, 10);
  c.setFont(&FreeSans9pt7b);
  c.setTextColor(BLACK);
  c.print(title);

  int x_center = (int)this->width / 2;
  int y_center = (int)((this->height - 15) / 2) + 15;
  if (x_center < y_center - 15)
  {
    outer_radius = x_center - 5;
    inner_radius = int(x_center * 0.7) ; //- 20;
  }
  else
  {
    outer_radius = y_center - 15;
    inner_radius = int(y_center * 0.7); //- 30;
  }

  c.fillCircle(x_center, y_center, outer_radius + 2, BLACK);
  c.fillCircle(x_center, y_center, inner_radius - 2, WHITE);
  c.fillTriangle(2, this->height -2, x_center, y_center, this->width -2, this->height -2 , WHITE);

  int count_r = get_data_range(records, range, range / 2);
  max = records[0].value;
  min = records[0].value;
  avg = 0;
  current_value = records[count_r - 1].value;
  for (int i = 0; i < count_r; i++)
  {
    avg += records[i].value;
    if (records[i].value > max)
    {
      max = records[i].value;
    };
    if (records[i].value < min)
    {
      min = records[i].value;
    };
  }
  avg /= count_r;

  // Arc

  int start_angle = -135;
  int seg_count = (int)((current_value - min_val) * 90) / this->max_val;

  byte seg = 3; // Segments are 3 degrees wide = 120 segments for 360 degrees
  byte inc = 3; // Draw segments every 3 degrees, increase to 6 for segmented ring

  // Draw colour blocks every inc degrees
  for (int i = start_angle; i < start_angle + seg * seg_count; i += inc)
  {
    // Calculate pair of coordinates for segment start
    float sx = cos((i - 90) * DEG2RAD);
    float sy = sin((i - 90) * DEG2RAD);
    uint16_t x0 = sx * (inner_radius) + x_center;
    uint16_t y0 = sy * (inner_radius) + y_center;
    uint16_t x1 = sx * outer_radius + x_center;
    uint16_t y1 = sy * outer_radius + y_center;

    // Calculate pair of coordinates for segment end
    float sx2 = cos((i + seg - 90) * DEG2RAD);
    float sy2 = sin((i + seg - 90) * DEG2RAD);
    int x2 = sx2 * inner_radius + x_center;
    int y2 = sy2 * inner_radius + y_center;
    int x3 = sx2 * outer_radius + x_center;
    int y3 = sy2 * outer_radius + y_center;
    if (current_value < thr1)
    {
      c.fillTriangle(x0, y0, x1, y1, x2, y2, GREEN);
      c.fillTriangle(x1, y1, x2, y2, x3, y3, GREEN);
    }
    else if (current_value < thr2)
    {
      c.fillTriangle(x0, y0, x1, y1, x2, y2, ORANGE);
      c.fillTriangle(x1, y1, x2, y2, x3, y3, ORANGE);
      this->alert = 1;
    }
    else
    {
      c.fillTriangle(x0, y0, x1, y1, x2, y2, RED);
      c.fillTriangle(x1, y1, x2, y2, x3, y3, RED);
      this->alert = 2;
    }
  }

  /* Threeshold*/
  c.setTextColor(BLACK);
  c.setCursor(x_center - (outer_radius * 0.85), 15 + (this->height * 0.85));
  c.setFont(&FreeSans9pt7b);
  c.print(this->min_val);
  c.setCursor(x_center + (outer_radius * 0.85) - 15, 15 + (this->height * 0.85));
  c.print(this->max_val);
  c.setFont(&FreeSans18pt7b);
  c.setCursor((this->width / 2) - 30, 15 + (this->height / 2));
  c.print((int)current_value);

  return true;
}

bool PrometheusClient::getStat(int range)
{
  this->alert = 0;
  float max, min, avg;
  BufferCanvas c(this->buffer, this->width, this->height);
  c.fillScreen(WHITE); // white background
  c.drawRect(0, 0, this->width, this->height, BLACK);
  c.setCursor(5, 10);
  c.setFont(&FreeSans9pt7b);
  c.setTextColor(BLACK);
  c.print(title);
  int count_r = get_data_range(records, range, range / ((this->width - 10) / 10)); // was 4
  max = records[0].value;
  min = records[0].value;
  avg = 0;
  // Get min, max and avg value
  for (int i = 0; i < count_r; i++)
  {
    avg += records[i].value;
    if (records[i].value > max)
    {
      max = records[i].value;
    };
    if (records[i].value < min)
    {
      min = records[i].value;
    };
  }
  avg /= count_r;
  c.setCursor(25, 50);
  c.setFont(&FreeSans18pt7b);
  float current_value = records[count_r - 1].value;
  c.setTextColor(BLACK);
  if (showthr)
  {
    if (current_value < thr1)
    {
      c.setTextColor(GREEN);
    }
    else if (current_value < thr2)
    {
      c.setTextColor(ORANGE);
      this->alert = 1;
    }
    else
    {
      c.setTextColor(RED);
      this->alert = 2;
    }
  }
  c.print((int)current_value);
  if (this->showtrend) {
  c.setTextColor(BLACK);
  c.setFont();
  c.setCursor(5, 60);
  c.print("Min");
  c.setCursor(40, 60);
  c.print("Max");
  c.setCursor(75, 60);
  c.print("Avg");
  c.setFont(&FreeSans9pt7b);
  c.setCursor(5, 85);
  c.print((int)min);
  c.setCursor(40, 85);
  c.print((int)max);
  c.setCursor(75, 85);
  c.print((int)avg);
  c.setFont();
  }
  return true;
}

bool PrometheusClient::getTimeseries(int range)
{
  this->alert = 0;
  float max, min;
  long ts_min, ts_max;
  int dy = (int)this->height / 10;
  int dx = (int)this->width / 10;
  if (dy < 10)
  {
    dy = 10;
  };
  if (dy > 30)
  {
    dy = 30;
  }
  if (dx < 10)
  {
    dx = 10;
  };
  if (dx > 45)
  {
    dx = 45;
  }
  BufferCanvas c(this->buffer, this->width, this->height);
  c.fillScreen(WHITE); // white background
  c.drawRect(0, 0, this->width, this->height, BLACK);
  c.drawLine(dx, 0, dx, this->height - dy, BLACK);
  c.drawLine(dx, this->height - dy, this->width - 1, this->height - dy, BLACK);
  c.drawLine(dx, dy, this->width - 1, dy, BLACK);
  c.setCursor(dx + 5, 1);
  c.setTextColor(BLACK);
  if (dy > 25)
  {
    c.setCursor(dx + 5, 18);
    c.setFont(&FreeSans12pt7b);
  }
  else if (dy > 14)
  {
    c.setCursor(dx + 5, 8);
    c.setFont(&FreeSans9pt7b);
  }
  c.print(title);
  c.setFont();
  // Get data
  int count_r = get_data_range(records, range, range / ((this->width - dx) / 10)); //was 4
  // Get Max and Min
  max = records[0].value;
  min = records[0].value;
  // Get min and max timestamp
  ts_min = records[0].timestamp;
  ts_max = records[count_r - 1].timestamp;
  // Draw vertical lines for hours
  struct tm timeinfo;
  int xp = -40;
  int ythr1, ythr2;

  // Get min and max value
  for (int i = 0; i < count_r; i++)
  {
    if (records[i].value > max)
    {
      max = records[i].value;
    };
    if (records[i].value < min)
    {
      min = records[i].value;
    };
  }
  long oldmin = min;
  long oldmax = max;

  long min_range = (long)(oldmin - ((max - min) / 5));
  long max_range = (long)(oldmax + ((max - min) / 5));
  long min_perc = (long)oldmin * 0.9;
  long max_perc = (long)oldmax * 1.1;
  if (min_range < min_perc)
  {
    min = min_range;
  }
  else
  {
    min = min_perc;
  };
  if (max_range > max_perc)
  {
    max = max_range;
  }
  else
  {
    max = max_perc;
  };

  float divider = (max - min) / (this->height - (2 * dy));
  if (divider < 0.05)
  {
    divider = 0.05;
  };
  /*
  Serial.print("min: ");
  Serial.print(min);
  Serial.print(" | max: ");
  Serial.print(max);
  Serial.print(" | Divider: ");
  Serial.println(divider);
*/

  // threshold
  if (showthr)
  {
    ythr1 = this->height - (((thr1 - min) / divider) + dy);
    ythr2 = this->height - (((thr2 - min) / divider) + dy);

    if (ythr1 < dy + 1)
    {
      ythr1 = dy + 1;
    }
    else if (thr1 > this->height)
    {
      ythr1 = this->height;
    }
    else
    {
      c.setCursor(0, ythr1);
      c.setTextColor(DARKGREY);
      if (dx > 20)
      {
        c.setCursor(2, ythr1 + 9);
        c.setFont(&FreeSans9pt7b);
      }
      c.print(thr1);
      c.setFont();
    }

    if (ythr2 < dy + 1)
    {
      ythr2 = dy + 1;
    }
    else if (thr2 > this->height)
    {
      ythr2 = this->height;
    }
    else
    {
      c.setCursor(0, ythr2);
      c.setTextColor(DARKGREY);
      if (dx > 20)
      {
        c.setCursor(2, ythr2 + 9);
        c.setFont(&FreeSans9pt7b);
      }

      c.print(thr2);
      c.setFont();
    }
    c.fillRect(dx + 1, dy + 1, this->width - 2, ythr2, PASTELRED);
    c.fillRect(dx + 1, ythr2, this->width - 2, ythr1 - ythr2, PASTELORANGE);
    c.fillRect(dx + 1, ythr1, this->width - 2, this->height - ythr1 - dy - 1, PASTELGREEN);
  }
  // Print min and max
/*
  Serial.print("ythr1: ");
  Serial.print(ythr1);
  Serial.print(" | ythr2: ");
  Serial.println(ythr2);
  */
  // Time
  for (long tt = (ts_min + 600); tt < (ts_max + 600); tt += 600)
  {
    time_t ti = (time_t)(tt - (tt % 600));
    gmtime_r(&ti, &timeinfo);
    int xi = getX(this->width, dx, ti, ts_min, ts_max);
    c.drawLine(xi, dy, xi, this->height - dy, LIGHTGREY);
    if ((xi - xp) > 40)
    {
      c.setCursor(xi - 10, this->height - dy + 2);
      if ((dx > 20) && (dy > 20))
      {
        c.setCursor(xi - 15, this->height - 10);
        c.setFont(&FreeSans9pt7b);
      }
      c.setTextColor(DARKGREY);
      c.print(timeinfo.tm_hour);
      c.print(":");
      c.print(timeinfo.tm_min);
      xp = xi;
      c.setFont();
    }
    /// GRID
    for (int j = dy + 1; j < this->height - dy; j += ((this->height - dy) / 5))
    {
      c.drawLine(dx + 1, j, this->width, j, LIGHTGREY);
      c.setCursor(0, j);
      c.setTextColor(DARKGREY);
      if (dx > 20)
      {
        c.setCursor(2, j + 9);
        c.setFont(&FreeSans9pt7b);
      }
      c.print((int)(((this->height - j - dy) * divider) + min));
      c.setFont();
    }

    if ((ti % 3600) == 0)
    {
      c.drawLine(xi + 1, dy, xi + 1, this->height - dy, DARKGREY);
      // c.drawLine(xi - 1, 10, xi - 1, this->height - 10, DARKGREY);
      c.setTextColor(BLACK);
      c.setCursor(xi - 10, this->height - dy + 2);
      if ((dx > 20) && (dy > 20))
      {
        c.setCursor(xi - 15, this->height - 10);
        c.setFont(&FreeSans9pt7b);
      }
      c.print(timeinfo.tm_hour);
      c.print(":00");
      c.setFont();
    }
  }

  // Draw
  int xx = dx;
  int yy = dy;
  int new_y, new_x;
  if (count_r > ((this->width - dx) / 2))
  {
    Serial.println("Truncating");
    count_r = ((this->width - dx) / 2);
  };
  for (int i = 0; i < count_r; i++)
  {
    new_y = this->height - (((records[i].value - min) / divider) + dy);
    if (new_y > (this->height - 1))
    {
      new_y = this->height;
    };
    if (new_y < 0)
    {
      new_y = 0;
    };
    new_x = getX(this->width, dx, records[i].timestamp, ts_min, ts_max);
    c.drawLine(xx, yy, new_x, new_y, BLUE);
    c.drawCircle(new_x, new_y, 2, BLUE);
    yy = new_y;
    xx = new_x;
  }
  if (records[count_r].value > this->thr2)
  {
    this->alert = 2;
  }
  else if (records[count_r].value > this->thr1)
  {
    this->alert = 1;
  }
  return true;
}

void PrometheusClient::clearBuffer()
{
  for (int i = 0; i < this->width * this->height; i++)
  {
    buffer[i] = 0;
  }
}

DynamicJsonDocument PrometheusClient::performPrometheusQuery(const char *metric, unsigned long windowSeconds, unsigned long step)
{
  WiFiClient client;
  HttpClient httpclient = HttpClient(client, this->prometheusHost, this->prometheusPort);

  client.setTimeout(2000);
  /// this->timeClient.forceUpdate();
  unsigned long endTs = timeClient->getEpochTime();
  unsigned long startTs = endTs - windowSeconds;
  /* to be moved */

  String query = "query=" + String(metric);
  query += "&start=" + String(startTs);
  query += "&end=" + String(endTs);
  query += "&step=" + String(step);
  query += "s";

  // Read response
  String contentType = "application/x-www-form-urlencoded";
  httpclient.post("/api/v1/query_range", contentType, query);

  String response;
  const size_t bufSize = 4096; // small chunk
  char buf[bufSize];
  response = httpclient.responseBody();
  DynamicJsonDocument doc(96000);
  auto err = deserializeJson(doc, response);
  if (err)
  {
    DynamicJsonDocument errDoc(256);
    errDoc["error"] = "json_parse_failed";
    return errDoc;
  }

  return doc;
}

int PrometheusClient::get_data_range(PrometheusClient::Record *arr, int range, int step)
{
  int count_r = 0;
  DynamicJsonDocument result = performPrometheusQuery(metric, range, step);
  // Print part of result
  if (result.containsKey("status") && result["status"] == "success")
  {
    JsonArray results = result["data"]["result"].as<JsonArray>();
    for (JsonObject item : results)
    {

      if (item.containsKey("metric") && item["metric"].containsKey("instance"))
      {
        //  //Serial.print("Instance: ");
        //  //Serial.println(item["metric"]["instance"].as<const char *>());
      }
      if (item.containsKey("values"))
      {
        JsonArray values = item["values"].as<JsonArray>();
        for (JsonArray pair : values)
        {
          unsigned long ts = pair[0];
          const char *val = pair[1];

          arr[count_r].timestamp = long(pair[0]);
          arr[count_r].value = float(pair[1]);
          count_r++;
        }
      }
    }
  }
  else
  {
    // Serial.println("Query failed: ");
    // Serial.println(result["data"]["result"]);
    // Serial.println("=== Prometheus result.data.result ===");
    serializeJsonPretty(result, Serial); // nicely formatted
    // Serial.println("\n===============================");
    if (result.containsKey("error"))
    {
      // Serial.println(result["error"].as<const char *>());
    }
  }
  return count_r;
}

bool PrometheusClient::drawOnBuffer(uint16_t *buffer, int16_t w, int16_t h)
{
  /* TO BE REMOVED*/
  return true;
}

int PrometheusClient::getX(int W, int xOffset, unsigned long ts, unsigned long start, unsigned long end)
{
  int x = xOffset + (int)(((ts - start) * (W - xOffset)) / (end - start));
  if (x < 0)
  {
    x = 0;
  };
  if (x > W)
  {
    x = W;
  };
  return x;
}