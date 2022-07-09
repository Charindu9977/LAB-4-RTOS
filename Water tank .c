#include "thingProperties.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <LiquidCrystal_I2C.h>

#define LED_BUILTIN 2
#define ANALOG_IN 34
#define OUTPUT_PIN 23
#define TRIG_PIN 5
#define ECHO_PIN 18
#define BUZZER_PIN 32
#define WATERPUMP_PIN 23

long duration;
long distance;

float depth = 13.0;
float Minimum_Level = 2;
float Analog_Value = 0;

static TaskHandle_t lcd_task = NULL;
static TaskHandle_t task_wifi = NULL;
static TaskHandle_t task_server = NULL;
static TaskHandle_t read_data = NULL;

LiquidCrystal_I2C lcd(0x27, 16, 2);

WebServer server(80);
WiFiClient client;

const char *ssid = "ESP32-WebServer";
const char *password = "12345678";


void LCD_Init() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Water Level:");
}

void lcd_print(void *parameter) {
  while (1) {
    lcd.setCursor(0, 1);
    lcd.print(sensor1);
    lcd.print("cm");
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void ArduinoCloud_Task(void *parameters) {
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();
  Serial.println("WIFI Access Point is Running");
  Serial.println(WiFi.localIP());
  while (1) {
    ArduinoCloud.update();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

String HTML(){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>Water Monitoring System Dashboard</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}\n";
  ptr +="p {font-size: 24px;color: #444444;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<div id=\"webpage\">\n";
  ptr +="<h1>Water Monitoring System</h1>\n";
  ptr +="<p>Water Level(%): ";
  ptr +=percentageWaterLevel;
  ptr +="%</p>";
  ptr +="<p>Water Level(cm): ";
  ptr +=sensor1;
  ptr +="<p>Pump Status: ";
    if (output_WaterPump) {
      ptr +="Pump is On";
  }
  else {
    ptr +="Pump is Off";
  }
  ptr +=sensor1;
  ptr +="</div>\n";
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}


void Web_Server(void *parameter){
  WiFi.mode(WIFI_AP);
  Serial.println("Setting up Access Point");
  WiFi.softAP(ssid, password);
  Serial.println("WIFI Access Point is Running");
  Serial.println(WiFi.softAPIP());
  if(MDNS.begin("esp32")){
    Serial.println("MDNS responder started");
  }
 server.on("/", handle_root);
 server.onNotFound(handleNotFound);
 server.begin();
 Serial.println("HTTP server started");
 while(1){
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println("Waiting For Hosts..");
  server.handleClient();
  delay(2);
 }
 vTaskDelete(NULL);
}

void handle_root(){
  server.send(200, "text/html", HTML());
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  server.send(404, "text/plain", message);
}

void Read_Data(void *parameter) {
  while (1) {
    delay(5);
    digitalWrite(TRIG_PIN, LOW);
    vTaskDelay(2 / portTICK_PERIOD_MS);
    digitalWrite(TRIG_PIN, HIGH);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    digitalWrite(TRIG_PIN, LOW);
    duration = pulseIn(ECHO_PIN, HIGH);
    distance = duration * 0.034 / 2;
    sensor1 = depth - distance;
    percentageWaterLevel = int(sensor1 / depth) * 100;
     vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

void Sound_Buzzer(void *parameter){
  while(1){
    if(sensor1 > depth){
      digitalWrite(BUZZER_PIN, HIGH);
    }else if(sensor1 < Minimum_Level){
      digitalWrite(BUZZER_PIN, HIGH);
    }else{
      digitalWrite(BUZZER_PIN, LOW);
    }
  }
}


void Create_ArduinoCloud_Task() {
  xTaskCreatePinnedToCore(
    ArduinoCloud_Task,
    "Connect to IOT Cloud",
    8024,
    NULL,
    1,
    NULL,
    0);
}

void Create_ReadData_Task() {
  xTaskCreatePinnedToCore(
    Read_Data,
    "Read Data",
    4024,
    NULL,
    1,
    &read_data,
    1);
}

void Create_lcd_print_task() {
  xTaskCreatePinnedToCore(
    lcd_print,
    "Print to Display",
    4024,
    NULL,
    1,
    &lcd_task,
    1);
}

void Create_Buzzer_Task() {
  xTaskCreatePinnedToCore(
    Sound_Buzzer,
    "Sound the Buzzer",
    2024,
    NULL,
    1,
    NULL,
    1);
}

void Create_WebServer_Task(){
    xTaskCreatePinnedToCore(
          Web_Server,
          "Web Server",
          4024, 
          NULL, 
          1, 
          &task_server, 
          1); 
}



void setup() {
  Serial.begin(9600);
  vTaskDelay(1500 / portTICK_PERIOD_MS);

  pinMode(WATERPUMP_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(OUTPUT_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);

  pinMode(ANALOG_IN, INPUT);
  pinMode(18, INPUT);
  pinMode(ECHO_PIN, INPUT);

  LCD_Init();
  Create_ArduinoCloud_Task();
  Create_ReadData_Task();
  Create_lcd_print_task();
  Create_Buzzer_Task();
  Create_WebServer_Task();
}

void loop() {

}


void onOutputWaterPumpChange()  {
  if (output_WaterPump) {
    digitalWrite(WATERPUMP_PIN, HIGH);
  }
  else {
    digitalWrite(WATERPUMP_PIN, LOW);
  }
}
