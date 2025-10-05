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
    timeClient = new NTPClient(*ntpUDP, "pool.ntp.org", 0, 60000);
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
  if (buffer)
  {
    delete[] buffer;
  }
  this->width = w;
  this->height = h;
  this->buffer = new uint16_t[width * height];
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

String PrometheusClient::getMetric()
{
  return this->metric;
}

void PrometheusClient::setthr1(long thr)
{
  thr1 = thr;
}

void PrometheusClient::setthr2(long thr)
{
  thr2 = thr;
}

void PrometheusClient::enableThr(bool enabled)
{
  showthr = enabled;
};

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

bool PrometheusClient::getTimeseries(int range)
{
  float max, min;
  long ts_min, ts_max;
  // timeClient.update();   // update NTP
  BufferCanvas c(this->buffer, this->width, this->height);
  c.fillScreen(WHITE); // white background
  c.drawLine(10, 0, 10, this->height - 10, BLACK);
  c.drawLine(10, this->height - 10, this->width - 1, this->height - 10, BLACK);
  c.drawLine(10, 10, this->width - 1, 10, BLACK);
  c.setCursor(15, 1);
  c.setTextColor(BLACK);
  c.print(title);
  // Get data
  int count_r = get_data_range(records, range, range / ((this->width - 10) / 4));
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
  long  oldmin = min;
  long oldmax = max;

  long min_range =(long) (oldmin - (( max - min) / 5));
  long max_range =(long) (oldmax + (( max - min) / 5));
  long min_perc = (long) oldmin * 0.9;
  long max_perc = (long) oldmax * 1.1;
  if (min_range < min_perc ) { min = min_range; } else {min = min_perc;};
  if (max_range > max_perc ) { max = max_range;} else {max = max_perc;};  

  float divider = (max - min) / (this->height - 20);
  if (divider < 0.001)
  {
    divider = 0.001;
  };
  Serial.print("min: ");
  Serial.print(min);
  Serial.print(" | max: ");
  Serial.print(max);
  Serial.print(" | Divider: ");
  Serial.println(divider);

  // threshold
  if (showthr)
  {
    ythr1 = this->height - (((thr1 - min) / divider) + 10);
    ythr2 = this->height - (((thr2 - min) / divider) + 10);

    if (ythr1 < 11)
    {
      ythr1 = 11;
    }
    else if (thr1 > this->height)
    {
      ythr1 = this->height;
    }
    else
    {
      c.setCursor(0, ythr1);
      c.setTextColor(DARKGREY);
      c.print(thr1);
    }

    if (ythr2 < 11)
    {
      ythr2 = 11;
    }
    else if (thr2 > this->height)
    {
      ythr2 = this->height;
    }
    else
    {
      c.setCursor(0, ythr2);
      c.setTextColor(DARKGREY);
      c.print(thr2);
    }
    c.fillRect(11, 11, this->width - 2, ythr2, PASTELRED);
    c.fillRect(11, ythr2, this->width - 2, ythr1 - ythr2, PASTELORANGE);
    c.fillRect(11, ythr1, this->width - 2, this->height - ythr1 - 11, PASTELGREEN);
  }
  // Print min and max
  /*c.setCursor(0, 11);
  c.setTextColor(BLUE);
  c.print(int(max));
  c.setCursor(0, this->height - 20);
  c.setTextColor(BLUE);
  c.print(int(min));*/
  Serial.print("ythr1: ");
  Serial.print(ythr1);
  Serial.print(" | ythr2: ");
  Serial.println(ythr2);
  // Time
  for (long tt = (ts_min + 600); tt < (ts_max + 600); tt += 600)
  {
    time_t ti = (time_t)(tt - (tt % 600));
    gmtime_r(&ti, &timeinfo);
    int xi = getX(this->width, 10, ti, ts_min, ts_max);
    c.drawLine(xi, 10, xi, this->height - 10, LIGHTGREY);
    if ((xi - xp) > 40)
    {
      c.setCursor(xi - 10, this->height - 8);
      c.setTextColor(DARKGREY);
      c.print(timeinfo.tm_hour);
      c.print(":");
      c.print(timeinfo.tm_min);
      xp = xi;
    }
    /// GRID
    for (int j = 11; j < this->height - 10; j += ((this->height - 10) / 5))
    {
      c.drawLine(11, j, this->width, j, LIGHTGREY);
      c.setCursor(0, j);
      c.setTextColor(DARKGREY);
      
      c.print((int) ( ( (this->height  - j - 10 ) * divider ) + min));
    }

    if ((ti % 3600) == 0)
    {
      c.drawLine(xi + 1, 10, xi + 1, this->height - 10, DARKGREY);
      // c.drawLine(xi - 1, 10, xi - 1, this->height - 10, DARKGREY);
      c.setTextColor(BLACK);
      c.setCursor(xi - 10, this->height - 8);
      c.print(timeinfo.tm_hour);
      c.print(":00");
    }
  }

  // Draw
  int xx = 10;
  int yy = 10;
  int new_y, new_x;
  if (count_r > ((this->width - 10) / 2))
  {
    count_r = ((this->width - 10) / 2);
  };
  for (int i = 0; i < count_r; i++)
  {
    new_y = this->height - (((records[i].value - min) / divider) + 10);
    if (new_y > (this->height - 1))
    {
      new_y = this->height;
    };
    if (new_y < 0)
    {
      new_y = 0;
    };
    new_x = getX(this->width, 10, records[i].timestamp, ts_min, ts_max);
    c.drawLine(xx, yy, new_x, new_y, BLUE);
    c.drawCircle(new_x, new_y, 2, BLUE);
    yy = new_y;
    xx = new_x;
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
  // PrometheusClient::drawTimeGrid(this->width,this->height,10,startTs,endTs);

  String query = "query=" + String(metric);
  query += "&start=" + String(startTs);
  query += "&end=" + String(endTs);
  query += "&step=" + String(step);
  query += "s";
  // //Serial.println(query);
  // Read response
  String contentType = "application/x-www-form-urlencoded";
  httpclient.post("/api/v1/query_range", contentType, query);

  String response;
  const size_t bufSize = 4096; // small chunk
  char buf[bufSize];

  response = httpclient.responseBody();
  ////Serial.println(response);

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
    //  //Serial.println(result["data"]["result"]);

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