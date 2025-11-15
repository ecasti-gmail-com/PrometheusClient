# PrometheusClient Library

A library to create images based on Prometheus query. Very handy to create dashboards on ESP32 based devices. 
Return a pointer to a buffer, who can be pushed, as an image in the main program. 


# Usage

Minimal setup 

    char* prometheushost = "<your_server>"; 
    int prometheusport = <your_port>;      // 9090 by default
    
    PrometheusClient c1;
    Image img;  // Buffer hosting the image result from Library
    LovyanGFX* gfx;
    setup()
      myBuffer = c1.init(w1, h1);
      img = Image(ENCODING_RGB16, (uint8_t*)myBuffer, w1, h1);
      c1.setMetric("100*avg(rate(node_cpu_seconds_total{mode='user'}[1m]))");
      c1.setTitle("Cpu util - %");
      c1.setThr1(50);
      c1.setThr2(75);
      c1.enableThr(true);
      c1.setHost( prometheushost, prometheusport);

    loop()
     c1.getTimeseries(3600);
     gfx->pushImage(5, 5, w1, h1, myBuffer);

