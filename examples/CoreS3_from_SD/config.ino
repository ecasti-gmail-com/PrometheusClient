/*
 * Inspired by: https://arduinogetstarted.com/tutorials/arduino-read-config-from-sd-card
 */




#define FILE_NAME "/config.txt"

#define KEY_MAX_LENGTH 40     // change it if key is longer
#define VALUE_MAX_LENGTH 150  // change it if value is longer

String *filedir[] = {};
int filenum = 0;
struct files {
  String name;
  int size;
};
struct files myfiles[100];



void init_sd() {
  SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
  if (!SD.begin(SD_SPI_CS_PIN, SPI, 25000000)) {
    // Print a message if SD card initialization failed or if the SD card does not exist.
    Serial.println("\n SD card not detected\n");
    while (1)
      ;
  } else {
    Serial.println("\n SD card detected\n");
  }
  delay(1000);
}


void read_config() {
  //init_sd();
  prometheushost = SD_findString("prometheushost");
  prometheusport = SD_findInt("prometheusport");
  ssid = SD_findString("ssid");
  password =  SD_findString("password");
  enablehttps = SD_findInt("enablehttps");

  int ww, hh;
  for (int i = 0; i < 5; i++) {
    panel[i].client = &clients[i];
    ww = SD_findInt(String((String(i) + "_w").c_str()));
    hh = SD_findInt(String((String(i) + "_h").c_str()));
    panel[i].myBuffer = panel[i].client->init(ww, hh);
    panel[i].metric = SD_findString((String(i) + "_metric"));
    panel[i].client->setMetric((char *) panel[i].metric.c_str());

    panel[i].title = SD_findString(String(i) + "_title");

    panel[i].client->setTitle((char *) panel[i].title.c_str());
    panel[i].client->setThr1(SD_findInt(String((String(i) + "_thr1").c_str())));
    panel[i].client->setThr2(SD_findInt(String((String(i) + "_thr2").c_str())));
    panel[i].client->enableThr(SD_findInt(String((String(i) + "_enableThr").c_str())));
    panel[i].client->enableTrend(SD_findInt(String((String(i) + "_enableTrend").c_str())));
    panel[i].type = SD_findInt(String((String(i) + "_type").c_str()));
    panel[i].w = ww;
    panel[i].h = hh;
    panel[i].x = SD_findInt(String((String(i) + "_x").c_str()));
    panel[i].y = SD_findInt(String((String(i) + "_y").c_str()));
    panel[i].timerange = SD_findInt(String((String(i) + "_timerange").c_str()));
    panel[i].client->setHost((char *)prometheushost.c_str(), prometheusport);
    panel[i].client->setHttps(enablehttps);
  }
}



bool SD_available(const __FlashStringHelper *key) {
  char value_string[VALUE_MAX_LENGTH];
  int value_length = SD_findKey(key, value_string);
  return value_length > 0;
}

int SD_findInt(const String &key) {
  char value_string[VALUE_MAX_LENGTH];
  int value_length = SD_findKey(key, value_string);
  return HELPER_ascii2Int(value_string, value_length);
}

float SD_findFloat(const __FlashStringHelper *key) {
  char value_string[VALUE_MAX_LENGTH];
  int value_length = SD_findKey(key, value_string);
  return HELPER_ascii2Float(value_string, value_length);
}

String SD_findString(const String &key) {
  char value_string[VALUE_MAX_LENGTH];
  int value_length = SD_findKey(key, value_string);
  return HELPER_ascii2String(value_string, value_length);
}

char *SD_findChar(const String &key) {
  Serial.print(key);
  Serial.print("  ");
  char value_string[VALUE_MAX_LENGTH];
  int value_length = SD_findKey(key, value_string);
  Serial.print("SD_findChar: ");
  Serial.println(value_string);
  return value_string;
}


int SD_findKey(const String &key, char *value) {
  File configFile = SD.open(FILE_NAME);

  if (!configFile) {
    Serial.print(F("SD Card: error on opening file "));
    Serial.println(FILE_NAME);
    return 0;
  }

  char key_string[KEY_MAX_LENGTH];
  char SD_buffer[KEY_MAX_LENGTH + VALUE_MAX_LENGTH + 1];  // 1 is = character
  int key_length = 0;
  int value_length = 0;

  // Flash string to string
  PGM_P keyPoiter;
  keyPoiter = reinterpret_cast<PGM_P>(key.c_str());
  byte ch;
  do {
    ch = pgm_read_byte(keyPoiter++);
    if (ch != 0)
      key_string[key_length++] = ch;
  } while (ch != 0);

  // check line by line
  while (configFile.available()) {
    int buffer_length = configFile.readBytesUntil('\n', SD_buffer, 100);
    if (SD_buffer[buffer_length - 1] == '\r')
      buffer_length--;  // trim the \r

    if (buffer_length > (key_length + 1)) {                  // 1 is = character
      if (memcmp(SD_buffer, key_string, key_length) == 0) {  // equal
        if (SD_buffer[key_length] == '=') {
          value_length = buffer_length - key_length - 1;
          memcpy(value, SD_buffer + key_length + 1, value_length);
          break;
        }
      }
    }
  }

  configFile.close();  // close the file
  return value_length;
}

int HELPER_ascii2Int(char *ascii, int length) {
  int sign = 1;
  int number = 0;

  for (int i = 0; i < length; i++) {
    char c = *(ascii + i);
    if (i == 0 && c == '-')
      sign = -1;
    else {
      if (c >= '0' && c <= '9')
        number = number * 10 + (c - '0');
    }
  }

  return number * sign;
}

float HELPER_ascii2Float(char *ascii, int length) {
  int sign = 1;
  int decimalPlace = 0;
  float number = 0;
  float decimal = 0;

  for (int i = 0; i < length; i++) {
    char c = *(ascii + i);
    if (i == 0 && c == '-')
      sign = -1;
    else {
      if (c == '.')
        decimalPlace = 1;
      else if (c >= '0' && c <= '9') {
        if (!decimalPlace)
          number = number * 10 + (c - '0');
        else {
          decimal += ((float)(c - '0') / pow(10.0, decimalPlace));
          decimalPlace++;
        }
      }
    }
  }

  return (number + decimal) * sign;
}

String HELPER_ascii2String(char *ascii, int length) {
  String str;
  str.reserve(length);
  str = "";

  for (int i = 0; i < length; i++) {
    char c = *(ascii + i);
    str += String(c);
  }

  return str;
}
