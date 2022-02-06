#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT20.h"

#if defined(ESP32)
  #include <WiFiMulti.h>
  WiFiMulti wifiMulti;
  #define DEVICE "ESP32"
#elif defined(ESP8266)
  #include <ESP8266WiFiMulti.h>
  ESP8266WiFiMulti wifiMulti;
  #define DEVICE "ESP8266"
#endif
#include <InfluxDbClient.h> // load the client library
#include <InfluxDbCloud.h> // only for InfluxDB Cloud: load SSL certificate and additional method call
#include "secrets.h" // load connection credentials
#define TZ_INFO "Asia/Kolkata"
#define DEVICE_ID "Environ1"
// InfluxDB client for InfluxDB Cloud API
InfluxDBClient client_cloud(INFLUXDB_CLOUD_URL, INFLUXDB_CLOUD_ORG, INFLUXDB_CLOUD_BUCKET, INFLUXDB_CLOUD_TOKEN, InfluxDbCloud2CACert);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_MOSI   13
#define OLED_CLK   14
#define OLED_DC    5
#define OLED_CS    15
#define OLED_RESET 4

//#define DHTPIN 2     // what digital pin the DHT22 is conected to
//#define DHTTYPE DHT11   // there are multiple kinds of DHT sensors

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

DHT20 DHT(&Wire);


int timeSinceLastRead = 0;
long influxTime;

void setup()
{
  Serial.begin(115200);
  if(!display.begin(SSD1306_SWITCHCAPVCC))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  DHT.begin(2, 12);  // select your pin numbers here
  display.clearDisplay();
  display.display();
  delay(100);
  wifiConnect();
  influxTime=millis()+20000;
    timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");
}

void loop()
{
  //AllPixels();
  //TextDisplay();
  //InvertedTextDisplay();
  //ScrollText();
  //DisplayChars();
  readAndDisplayHumidity();
}
void readAndDisplayHumidity()
{
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  delay(20);
  DHT.read();
  float h,t;
  timeSinceLastRead=2001;
  if(timeSinceLastRead > 2000) {
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    h = DHT.getHumidity();
    // Read temperature as Celsius (the default)
    t = DHT.getTemperature();

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t)) {
      display.clearDisplay();
      display.println("Failed to read from DHT sensor!");
      timeSinceLastRead = 0;
      display.display();
      delay(1000);
    }
    display.clearDisplay();
    display.print("H: ");
    display.print(h);
    display.print(" %\n");
    display.print("T: ");
    display.print(t);
    display.print(" *C ");
    timeSinceLastRead = 0;
        display.display();
    delay(2000);
  }
      if(millis()-influxTime>=20000)
    {
      sendInflux(t,h);
      influxTime=millis();
    }
  delay(100);
}
void wifiConnect() {
   Serial.println("Scanning Wifi Networks...");
   int n = WiFi.scanNetworks();
   Serial.print(n);Serial.println("Wifi networks found. Trying to connect to 2 known networks");
   for (int i = 0; i < n; ++i) {
     Serial.println(WiFi.SSID(i));
     if (WiFi.SSID(i)== WIFI_SSID1) {
        WiFi.begin(WIFI_SSID1,WIFI_KEY1);
        break;
     }
     if (WiFi.SSID(i)== WIFI_SSID2) {
       WiFi.begin(WIFI_SSID2,WIFI_KEY2);
       break;
     }
    }
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Successfully connected to ");
  Serial.println(WiFi.SSID());
}
void sendInflux(float t, float h)
{
  // END: read sensor values
  Point pointDevice("environ"); // create a new measurement point (the same point can be used for Cloud and v1 InfluxDB)
  // add tags to the datapoints so you can filter them
  pointDevice.addTag("device", DEVICE_ID);
  pointDevice.addTag("SSID", WiFi.SSID());
  // Add data fields (values)
  pointDevice.addField("temperature1", t);
  pointDevice.addField("humidity1", h);
  pointDevice.addField("uptime", millis()); // in addition send the uptime of the Arduino
  pointDevice.addField("Strength",WiFi.RSSI());


  // Check server connection
  if (client_cloud.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client_cloud.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client_cloud.getLastErrorMessage());
  }
  Serial.println("writing to InfluxDB Cloud... ");
  if (!client_cloud.writePoint(pointDevice)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client_cloud.getLastErrorMessage());
  }
  else
  {
    Serial.println("Success");
  }

}
