#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <math.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
unsigned long temperatureDelay = 1000;
const char *ssid = "your SSID";
const char *password = "your PASSWORD";

WebServer server(80);


// Create a sensor object
Adafruit_MPU6050 mpu;
float tiltx , tilty, tiltz;
sensors_event_t a, g, temp;

// Init MPU6050
void initMPU(){
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");
}




void handleRoot() {
  char msg[3000];
  String accStr = getAccReadings();
  String tiltStr = computeTiltReading();
  snprintf(msg, 3000,
           "<html>\
  <head>\
    <meta http-equiv='refresh' content='1'/>\
    <meta name='viewport' content='width=device-width, initial-scale=1'>\
    <link rel='stylesheet' href='https://use.fontawesome.com/releases/v5.7.2/css/all.css' integrity='sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr' crossorigin='anonymous'>\
    <title>ESP32 IMU server</title>\
    <style>\
    html { font-family: helvetica; display: inline-block; margin: 0px auto; text-align: center;}\
    h2 { font-size: 3.0rem; }\
    p { font-size: 3.0rem; }\
    .units { font-size: 1.2rem; }\
    .dht-labels{ font-size: 1.5rem; vertical-align:middle; padding-bottom: 15px;}\
    </style>\
  </head>\
  <body>\
      <h2>ESP32 IMU Server!</h2>\
      <p>\
        <i class='fas fa-thermometer-half' style='color:#ca3517;'></i>\
        <span class='imu-labels'>temperature</span>\
        <span>%.2f</span>\
        <sup class='units'>&deg;C</sup>\
      </p>\
      <p>\
        <i class='fas fa-compass' style='color:#ffcc00;'></i>\
        <span class='imu-labels'>Acceleration</span><br>\
        <span>%s</span>\
      </p>\
      <p>\
        <i class='fas fa-balance-scale' style='color:#ffcc00;'></i>\
        <span class='imu-labels'>Tilt</span><br>\
        <span>%s</span>\
      </p>\
  </body>\
</html>",
           readTemperature(),accStr.c_str(),tiltStr.c_str()
          );
  server.send(200, "text/html", msg);
}

void setup(void) {

  Serial.begin(115200);

  Wire.begin(); 
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");


  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  initMPU();

  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }
  server.on("/", handleRoot);

  server.begin();
  Serial.println("HTTP server started");
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  delay(2000);
  
}

void loop(void) {
  server.handleClient();
 
  delay(2);//allow the cpu to switch to other tasks
float temperature = temp.temperature;
float accX = a.acceleration.x;
float accY = a.acceleration.y;
float accZ = a.acceleration.z;

float tiltx = atan2(-accX, sqrt(accY * accY + accZ * accZ)) * 180.0 / PI;
float tilty  = atan2(accY, accZ) * 180.0 / PI;
  displaySensorData(temperature, accX, accY, accZ, tiltx, tilty);
}


float readTemperature() {
  // Sensor readings may also be up to 2 seconds
  // Read temperature as Celsius (the default)
  float temperature;

  mpu.getEvent(&a, &g, &temp);
  temperature = temp.temperature;
  return temperature;
}


String getAccReadings() {
  mpu.getEvent(&a, &g, &temp);

  // Extract acceleration components
  float accX = a.acceleration.x;
  float accY = a.acceleration.y;
  float accZ = a.acceleration.z;

  // Format string with space separation
  String accString = String(accX, 2) + " " + String(accY, 2) + " " + String(accZ, 2);
  return accString;
}

String computeTiltReading(){

  mpu.getEvent(&a, &g, &temp);

   float ax = a.acceleration.x;
  float ay = a.acceleration.y;
  float az = a.acceleration.z;

  float tiltx = atan2(ay, az)* (180.0 / PI) ;
  float tilty = atan2(-ax, sqrt(ay * ay + az * az))* (180.0 / PI);

  String tiltString = String(tiltx, 2) + " " + String(tilty, 2);
  return tiltString;
}

void displaySensorData(float temperature, float accX, float accY, float accZ, float tiltx, float tilty) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);

  // Temperature
  display.print("Temp: ");
  display.print(temperature, 1);
  display.print(" C");

  // Acceleration
  display.setCursor(0, 12);
  display.setTextColor(SSD1306_WHITE);  
  display.print("Acc X:");
  display.print(accX, 2);
  display.print(" Y:");
  display.print(accY, 2);
  display.setCursor(0, 22);
  display.setTextColor(SSD1306_WHITE);
  display.print("Z:");
  display.print(accZ, 2);

  // Tilt
  display.setCursor(0, 35);
  display.setTextColor(SSD1306_WHITE);
  display.print("tiltx:");
  display.print(tiltx, 1);
  display.setCursor(0, 45);
  display.setTextColor(SSD1306_WHITE);
  display.print("tilty :");
  display.print(tilty, 1);

  display.display();
  delay(1000);
}

