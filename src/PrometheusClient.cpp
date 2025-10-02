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
  // timeClient.begin();
  Serial.print("Buffer address = 0x");
  Serial.println((uintptr_t)buffer, HEX);
  return buffer;
}

void PrometheusClient::setMetric(char *metric_p)
{
  this->metric = metric_p;
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

  String query = "query=" + String(metric);
  query += "&start=" + String(startTs);
  query += "&end=" + String(endTs);
  query += "&step=" + String(step);
  query += "s";
/*
  if (!client.connect(prometheusHost, prometheusPort))
  {
    DynamicJsonDocument errDoc(256);
    errDoc["error"] = "connect_failed";
    return errDoc;
  }

  String request = "POST /api/v1/query_range HTTP/1.1\r\n";
  request += "Host: " + String(prometheusHost) + "\r\n";
  request += "Content-Type: application/x-www-form-urlencoded\r\n";
  request += "Content-Length: " + String(query.length()) + "\r\n";
  request += "Connection: close\r\n";
  request += "Accept-Encoding: identity\r\n";
  request += "\r\n";
  request += query;
  // request += query;
  Serial.println(request);
  client.println(request);
  client.println();
 
  */

  // Read response
  String contentType = "application/x-www-form-urlencoded";
  String postData = "name=Alice&age=12";
  httpclient.post("/api/v1/query_range", contentType, query);

  String response;
  const size_t bufSize = 4096; // small chunk
  char buf[bufSize];

  response = httpclient.responseBody();
  Serial.println(response);
  /*
  while (client.connected() || client.available())
  {
    size_t len = client.readBytes(buf, bufSize);
    response += String(buf).substring(0, len);
  }
    while (client.connected() || client.available())
  {
    size_t len = client.readBytes(buf, bufSize);
    response += String(buf).substring(0, len);
  }
  client.stop();
  Serial.println(response);
  int bodyIndex = response.indexOf("\r\n\r\n");
  if (bodyIndex >= 0)
  {
    response = response.substring(bodyIndex + 4);
  }
  int firstClosingBracket = response.indexOf('{');
*/

  DynamicJsonDocument doc(96000);
  //  auto err = deserializeJson(doc, response.substring(firstClosingBracket));
  auto err = deserializeJson(doc, response);
  if (err)
  {
    DynamicJsonDocument errDoc(256);
    errDoc["error"] = "json_parse_failed";
    return errDoc;
  }

  return doc;
}

int PrometheusClient::get_data(PrometheusClient::Record *arr)
{
  int count_r = 0;
  // Example usage
  DynamicJsonDocument result = performPrometheusQuery(metric, 6000,30);
  // Print part of result
  if (result.containsKey("status") && result["status"] == "success")
  {
    JsonArray results = result["data"]["result"].as<JsonArray>();
    for (JsonObject item : results)
    {

      if (item.containsKey("metric") && item["metric"].containsKey("instance"))
      {
        Serial.print("Instance: ");
        Serial.println(item["metric"]["instance"].as<const char *>());
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
    Serial.println("Query failed: ");
    // Serial.println(result["data"]["result"]);

    Serial.println("=== Prometheus result.data.result ===");
    serializeJsonPretty(result, Serial); // nicely formatted
    Serial.println("\n===============================");

    if (result.containsKey("error"))
    {
      Serial.println(result["error"].as<const char *>());
    }
  }
  return count_r;
}

bool PrometheusClient::drawOnBuffer(uint16_t *buffer, int16_t w, int16_t h)
{
  if (!buffer || w <= 0 || h <= 0)
    return false;
  int count_r = get_data(records);
  BufferCanvas c(this->buffer, this->width, this->height);
  c.fillScreen(0xFFFF); // white background
  int xx = 0;
  int yy = 0;
  for (int i = 0; i < count_r; i++)
  {
    c.drawLine(xx, yy, i, records[i].value * 100, 0x0000);
    yy = records[i].value * 100;
    xx = i;
  }

  return true;
}