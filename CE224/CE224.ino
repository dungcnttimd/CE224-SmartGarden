/*
KHAI BÁO PIN
*/
#define LED 2     //D4  led nguồn
#define MOTOR 13  //D7
#define DHTPIN 12  //D6
#define DOAMDAT A0  //analog
#define TOUCH 16  //D0
//#define SDA 5  //D1
//#define SCL 4  //D2

/*
KHAI BÁO THƯ VIỆN
*/
/*---------HIỂN THỊ---------*/
//#include <Wire.h>
#include <LiquidCrystal_I2C.h>
/*---------KẾT NỐI---------*/
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp8266.h>
//web server 
#include <ThingSpeak.h>
/*---------CẢM BIẾN---------*/
#include <DHT.h>
#include <DHT_U.h>

/*
KHAI BÁO BIẾN TOÀN CỤC
*/
//BLYNK
//#define BLYNK_TEMPLATE_ID           "TMPLxxxxxx"
#define BLYNK_DEVICE_NAME "My_System"
#define BLYNK_AUTH_TOKEN "a64YarnJsGcQ_jrmC1L0jJARj84M7u3y"
#define BLYNK_PRINT Serial
char auth[] = BLYNK_AUTH_TOKEN;
//THINKSPEAK
const char* server = "api.thingspeak.com";
unsigned long myChannelNumber = 1433980;
const char * myWriteAPIKey = "0B2TBIQVLGHX2V5X";
const char * myReadAPIKey = "C89KRRWFFEIGBWOE";

WiFiClient client;

// char ssid[] = "FPT Ngoc Nhan";
// char pass[] = "khongnoiduoc";

char ssid[] = "ThanhTung";
char pass[] = "984513194";

LiquidCrystal_I2C lcd(0x27,16,2);          //Khởi tạo biến lcd
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);                 //Khởi tạo biến dht

bool motorStatus = 0;
bool valTouch = 0;
/*
HÀM KHỞI TẠO
*/

void setup() {
  Serial.begin(115200);
  
  //Debug hardware
  debugHardware();
  
  //INIT LCD
  lcd.init();
  lcd.backlight();
  //Test lcd
  lcd.setCursor(4,0);
  lcd.print("WELLCOME");
  lcd.setCursor(2,1);
  lcd.print("TO MY SYSTEM");
  //Wifi connection
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println(WiFi.localIP());
  //Thinkspeak init
  ThingSpeak.begin(client);
  //lcd wifi connected
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Wifi Connected");
  lcd.setCursor(0,1);
  lcd.print(WiFi.localIP());

  //init blynk
  Blynk.begin(auth, ssid, pass);
  dht.begin();
  
  pinMode(DOAMDAT, INPUT);
  pinMode(DHTPIN, INPUT);
  pinMode(TOUCH, INPUT);
  pinMode(MOTOR, OUTPUT);
  
  /*START*/
  digitalWrite(MOTOR, LOW);
  delay(1500);
}
//Đọc giá trị nút nhấn V0 //trigger
BLYNK_WRITE(V0){
  motorStatus = param.asInt();
}
/*
HÀM LOOP
*/
void loop() {
  //lear lcd
  lcd.clear();
  
  //run blynk
  Blynk.run();
  
  //-------- Cảm biến độ Soil
  float Soil = getSoil();
  //-------- Cảm biến độ DHT
  int h = dht.readHumidity();         //Độ 'ẩm'
  float t = dht.readTemperature();      //Độ '`C'
  if(!getDHT(h,t)){return;}
  //
//  motorStatus = getDataBlynk();
  //-------- Cảm biến touch
  getTouch();
  
  //Thiết lập ngưỡng tưới tự động theo độ ẩm đất
  ThresholdSoilMoisture(Soil);
  //-------- Điều khiển motor
  digitalWrite(MOTOR, motorStatus);

  //-------- Hiển thị ra lcd
  lcdRun(h, t, Soil, motorStatus);
  
  //-------- Gửi dữ liệu lên blynk
  sendDataBlynk(h, t, Soil, motorStatus);
  
  ////Debug serial
  //debugValue(h,t,Soil);

  
  //Think speak send data to web server
  thinkspeakRun(h, t, Soil, motorStatus);
  //-------- Hiển thị ra lcd
  lcdRun(h, t, Soil, motorStatus);
  
//  delay(70);    //Delay for control lcd
}

void debugValue(int h, float t, int Soil){
    //Debug data
  Serial.print("Nhiet do:  ");
  Serial.print(t);
  Serial.println(" `C");
  Serial.print("Do am:     ");
  Serial.print(h);
  Serial.println(" %");
  Serial.print("Do am dat: ");
  Serial.print(Soil);
  Serial.println(" %");
  Serial.println("");
}
void sendDataBlynk(int h, float t, int s, bool myMotor){
  Blynk.virtualWrite(V0, myMotor);
  Blynk.virtualWrite(V1, t);
  Blynk.virtualWrite(V2, h);
  Blynk.virtualWrite(V3, s);
}
void ThresholdSoilMoisture(int doamdat){
  //60 - 70 is perfect
  int minThreshold = 50;
  int maxThreshold = 90;
  if(doamdat < minThreshold){motorStatus = 1;}
  if(doamdat >= maxThreshold){motorStatus = 0;}
}
bool getDHT(int h, float t){
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return 0;
  }
  return 1;
}
int getSoil(){
  //-------- Cảm biến độ ẩm đất
  int doam = 0;
  int doamVal = 0;
  for(int i = 0; i<10; i++){
    doam += analogRead(DOAMDAT);
  }
  doam = doam/10;
  doamVal = map(doam, 400, 1024, 100, 0);
  if(doamVal >= 100){
     doamVal = 100;
  }else{
    if(doamVal <= 0){
      doamVal = 0;
    }
  }
  return doamVal;
}
void getTouch(){
  int Touch = analogRead(TOUCH);
  int thresholdTouch = 100;
  if(Touch >=  thresholdTouch){
      motorStatus = !motorStatus; 
      while(Touch >=  thresholdTouch){
        if(Touch <  thresholdTouch){break;}
        Touch = analogRead(TOUCH);
      }
  }
}
void debugHardware(){
  //Debug hardware
  pinMode(LED, OUTPUT);
  for(int i = 0; i<3; i++){
    digitalWrite(LED, HIGH);
    delay(200);
    digitalWrite(LED, LOW);
    delay(200);
  }
  digitalWrite(LED, LOW);
}
void thinkspeakRun(int h, float t, int s, bool motorStatus){
  if (client.connect(server,80)){ 
    ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    ThingSpeak.setField(1, t); 
    ThingSpeak.setField(2, h);
    ThingSpeak.setField(3, s);
    ThingSpeak.setField(4, motorStatus);
  }
  client.stop();
}
void lcdRun(int h, float t, int s, bool motorStatus){
  lcd.clear();
  
  lcd.setCursor(0,0);
  lcd.print("T:");
  lcd.setCursor(2,0);
  lcd.print(t);
  lcd.setCursor(6,0);
  lcd.print("'C  H:");
  if(h<10){
    lcd.setCursor(13,0);
    lcd.print(h);
    lcd.setCursor(14,0);
    lcd.print("%");
  }else{
    if(h!=100){
      lcd.setCursor(12,0);
      lcd.print(h);
      lcd.setCursor(14,0);
      lcd.print("%");
    }
    else{
      lcd.setCursor(12,0);
      lcd.print("100%");
    }
  }
  lcd.setCursor(0,1);
  lcd.print("RL:");
  lcd.setCursor(3,1);
  if(motorStatus){
    lcd.print("ON");
  }else{
    lcd.print("OFF");
  }
  lcd.setCursor(6,1);
  lcd.print(" Humi:");
  
  if(s<10){
    lcd.setCursor(13,1);
    lcd.print(s);
    lcd.setCursor(14,1);
    lcd.print("%");
  }else{
    if(s!=100){
      lcd.setCursor(12,1);
      lcd.print(s);
      lcd.setCursor(14,1);
      lcd.print("%");
    }
    else{
      lcd.setCursor(12,1);
      lcd.print("100%");
    }
  }
//  delay(10);
}
