/*
// Generic draw function
bool drawOnBuffer(uint16_t *buffer, int16_t w, int16_t h) {
  if (!buffer || w <= 0 || h <= 0) return false;

  int count_r = get_data(records);

  class BufferCanvas : public Adafruit_GFX {
  public:
    BufferCanvas(uint16_t *buf, int16_t wid, int16_t hei)
      : Adafruit_GFX(wid, hei), buf(buf) {}

    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
      if (x < 0 || y < 0 || x >= _width || y >= _height) return;
      buf[y * _width + x] = color;
    }

  private:
    uint16_t *buf;
  };

  BufferCanvas c(buffer, w, h);

  // Clear buffer
  c.fillScreen(0xFFFF);  // white background
   int xx = 0;
   int yy = 0;
  // Draw some example graphics
  //  c.drawCircle(w / 2, h / 2, min(w, h) / 3, 0xF800);  // red circle
  // c.drawLine(0, h, w, 0, 0x07E0);
  // c.drawLine(0, 0, w, h, 0x07E0);
  // c.drawPixel(2, 2, 0x07E0);  // green pixel
  for (int i = 0; i < count_r; i++) {
   // c.drawPixel(i, records[i].value * 100, 0x0000);
    c.drawLine(xx,yy,i,records[i].value * 100,0x0000);
    yy = records[i].value * 100;
    xx = i ;
  }




  return true;
}


void draw_sample() {

  if (drawOnBuffer(myBuffer, BUF_W, BUF_H)) {
    Serial.begin(115200);
    Serial.println("Draw succeeded");

    // Wrap buffer in an Image object
    Image img(ENCODING_RGB16, (uint8_t *)myBuffer, BUF_W, BUF_H);

    // Draw it on the screen at position (200,150)
    Display.begin();
    Display.fill(0x0000);
    Display.image(img, 100, 100);
    Display.endDraw();
  }
}



void init_display() {
  Display.begin();

  // Allocate a smaller buffer (100x100 pixels = 20KB in RGB565)

  static uint16_t myBuffer[BUF_W * BUF_H];

  // Draw into it
  if (drawOnBuffer(myBuffer, BUF_W, BUF_H)) {
    Serial.begin(115200);
    Serial.println("Draw succeeded");

    // Wrap buffer in an Image object
    Image img(ENCODING_RGB16, (uint8_t *)myBuffer, BUF_W, BUF_H);

    // Draw it on the screen at position (200,150)
    Display.image(img, 100, 100);
    Display.endDraw();
  }
}

*/
