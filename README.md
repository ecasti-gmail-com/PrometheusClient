# PrometheusClient Library

# 📊 Prometheus Display Library for ESP32

### Display Prometheusmetrics as **Graphs**, **Gauges**, or **Stats** on any ESP32-based device

This Arduino library lets you **query**  Prometheus (local or remote, HTTP or HTTPS) and render the results as PNG images on your display.  
It supports **ESP32 devices**, including M5Stack devices such as **M5Tab5**, **CoreS3**, and **AtomS3**.

You choose the metrics → the library renders the image → you push it to your screen using LovyanGFX (or your preferred display driver).
The scope is to have small and inexpensive device in your desk to have your most important Prometheus metrics at a glance

----------

## 🚀 Features

-   ✔️ Query any **Prometheus** instance (HTTP or HTTPS)
    
-   ✔️ Works with **Grafana Cloud** (username + API token supported)
    
-   ✔️ Generates **three types of metric panels**:
    
    -   **Time Series Graph**
        
    -   **Gauge**
        
    -   **Simple Stat**
        
-   ✔️ Supports Warning & Critical **threshold visualization**
    
-   ✔️ Min/Max visualization (optional)
    
-   ✔️ SD-card based configuration supported
    
-   ✔️ Touch-zoom view for full-screen metric
    
-   ✔️ LovyanGFX compatible (but adaptable to any display driver)
    
-   ✔️ Examples included for:
    
    -   **M5Tab5**
        
    -   **M5CoreS3**
        
    -   **M5AtomS3**
        

----------


## 📦 Hardware Requirements

- M5Stack device mentioned above 

or  

- Any **ESP32** board (PSRAM recommended but not required)
    
-   A display supported by **LovyanGFX** (or a custom driver)
    
-   Optional:
    
    -   SD card (max 8 GB recommended)
        
    -   Touch input for zoom features
        

**Note:** Arduino Giga support was previously included, but is temporarily disabled.  
It will be re-enabled in a future release.

📌 Technically, the library itself doesn't require a display, as an example you can create a simple webserver showing the dashboards via  web, or sending you the panel via Telegram when you reach a critical value, but I will not advertise for that, Grafana is way better generating Dashboards to be consumed via browser.

----------

# 📥 Installation

### 1. Download the ZIP

Get the `.zip` from this repository.

### 2. Install via Arduino IDE

`Sketch → Include Library → Add .ZIP Library` 

After installation, you will find **three full examples** inside the library examples folder.

----------

----------

# 🧰 Example Projects Included

**M5AtomS3_simple**

M5AtomS3 :  One panel demo, no psram and tiny screen

**CoreS3_from_SD**

M5CoreS3 : 5-panel dashboard loading config from SD card. Touch is enabled allowing to zoom in any panel

**M5Tab5_from_SD**

M5Tab5: 9 panel dashboard loading config from SD card. Touch is enabled allowing to zoom in any panel. 
In the bottom right side will show an image from according to max value of getAlert().
Images are normal.png, warning.png and critical.png with size 350x350 pixels. You can customize with your logo.



# 🧪 Example: Simple (AtomS3)

This minimal example works on **any ESP32 board** with LovyanGFX.

### Wi-Fi & Prometheus setup

`const  char* ssid = "< YOUR SSID >";
 const  char* password = "< YOUR PASSWORD >"; 
 char* prometheushost = "<YOUR SERVER>"; 
 int prometheusport = 9090;` 

### Define the metric

    c1.setMetric("100*avg(rate(node_cpu_seconds_total{mode='user'}[1m]))");
    c1.setTitle("Cpu util - %");
    c1.setThr1(50);
    c1.setThr2(75);
    c1.enableThr(true);

Compile → upload → you will see a **time-series graph** within ~1 second.

----------

# 📁 Example: SD-based Configuration (Tab5 / CoreS3)

For bigger dashboards (5–9 panels):

1.  Copy all files from the `SD/` folder to a FAT-formatted SD card (≤ 8 GB)
    
2.  Rename:
    

`config_M5Tab5.txt → config.txt or config_M5CoreS3.txt → config.txt` 

3. Customize the value for WiFi, Prometheus host and the metrics from 0 to 4  ( or 9) 

### Example config block

   

        ssid=<YOUR SSID> 
        password=<YOUR PASSWORD> 
        prometheushost="< YOUR PASSWORD >" 
        prometheusport=9090  
        enablehttps=0  
        # Metric  
        0_metric=100*avg(rate(node_cpu_seconds_total{mode='user'}[1m])) 
        # Title  
        0_title=Cpu utilization 
        # Type: 0=Gauge, 1=Stats, 2=TimeSeries  
        0_type=1  
        # Image size  
        0_w=250  
        0_h=300  
        # Image position on the screen  
        0_x=0  
        0_y=50  
        # Thresholds  
        0_thr1=50  
        0_thr2=80  
        0_enableThr=1  
        # Trend arrow  
        0_enableTrend=0  
        # Time range (seconds)  
        0_timerange=3600` 

📝 The **last image index** (`5_` or `9_`) is automatically used for **zoom mode**.

----------

# 🔐 HTTPS & Authentication (Grafana Cloud Example)

To use Grafana Cloud:

    `myBuffer10 = c10.init(w10, h10);
    c10.setMetric("heap_free_bytes/1000");
    c10.setTitle("Free mem KB - Grafana Cloud");
    
    c10.setHttps(true);
    c10.setCredentials("1234567", "glc_your_token_here");
    c10.setHost("prometheus-1234-prod-eu-west-2.grafana.net", 443);
    c10.setURI("/api/prom/api/v1/query_range");
    
    c10.setThr1(215);
    c10.setThr2(235);
    c10.enableThr(true);` 

### Notes

-   Grafana Cloud username is provided by the platform
    
-   Password = API Token with metric-read permissions
    
-   Prometheus-in-Kubernetes may require HTTPS without authentication
    

----------

# 🔧 API Reference (Template)



### Initialization

`uint8_t* init(int width, int height);` 
It returns a pointer to the buffer where it draws the image

### Metric & Title

    void  setMetric(const  char* qlQuery); 
    void  setTitle(const  char* title);

### Network

    void  setHost(const  char* host, int port);
Set the hostname (or IP) of prometheus server

     void  setURI(const  char* endpoint); 
Set the URI of the API endpoint, `/api/v1/query_range` by default

     void  setHttps(bool enable); 
Enable Https client. 
📌 Note: currently is not verifying the Certificate with a valid CA, but uses **setInsecure()**
 
 

    void  setCredentials(const  char* user, const  char* pass);
    
   Set username and password to access prometheus. Not always required on premise or at home, but required in the cloud.

### Rendering Settings

    void setMinVal(long min_val_p);
    
 Used only on the Gauge panel, setting the min value for the Gauge, default 0 
 
    void setMaxVal(long max_val_p); 
   Used only on the Gauge panel, setting the max value for the Gauge, default 100 
    
    void  enableTrend(bool enable); 
Enable the visualization of minimum and maximum value of the metric in the time range. Used only on Stats panel


### Rendering
    bool getTimeseries(int range);
Render the result as timeseries. 
Vertical and Horizontal range are dynamic, and will plot width/10 points
If Threshold is enabled, will draw the different areas in Green, Orange or Red. 


    bool getStat(int range);
Render a simple stat panel with last value from the query result. The size of the rendering is fixed 100x100, higher size of the buffer will be unused. 
If **threshold** is enabled, the Current Value will be rendered in Green, Orange or Red according to the thresholds configured. 
If **trend** is enabled, will print in the bottom the min and the max value in the range configured
    
    bool getGauge(int range);
Render a Gauge with current value in the range from min and max configured using **setMinVal** and **setMaxVal**
The Current value is also printed in the middle for an easy reading.
If Threshold is enabled, will draw the gauge in  in Green, Orange or Red according to the thresholds configured.

### Alert

     void setThr1(long thr);
     
   Set the threshold 1 (Warning Level) for the panel
   
     void setThr2(long thr);
     
   Set the threshold 2 (Critical Level) for the panel
   

    long getThr1();
Get the configured value of the threshold 1 

    long getThr2();

Get the configured value of the threshold 2

    void enableThr(bool enabled);
Enable threshold check

    bool getEnabledThr();

Return true if the treshold are enabled 

      int getAlert();

Return 0 if the current value did not reach any threshold, and 1 or 2 if reached the threshold 1 and threshould 2. 
You can call this metod after a refresh to enable some kind of notification.

 

     String getAlertString();

Return literaly **Normal**, **Warning** or **Critical**




----------

# 📄 License

Apache-2.0 




