#include <dht_nonblocking.h>
#define DHT_SENSOR_TYPE DHT_TYPE_11
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// DHT11 setup
static const int DHT_SENSOR_PIN = 2;
DHT_nonblocking dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

// Potentiometer pin
const int potPin = A0;

// Timing intervals
const unsigned long SENSOR_INTERVAL      = 3000;    // 3 s
const unsigned long TIME_SYNC_INTERVAL   = 300000;  // 5 min
const unsigned long ONE_SECOND          = 1000;

unsigned long lastSensorRead      = 0;
unsigned long lastTimeSync       = 0;
unsigned long lastDisplayRefresh = 0;

// Environment readings
float temperature = 0, humidity = 0;

// Time variables
char  dayOfWeek[4], monthStr[4];
int   day, hour, minute, second, year;

//─────────────────────────────────────────────────────────────────────────────
// Helpers from your baseline
double mapValue(double x,double in_min,double in_max,double out_min,double out_max){
  return (x - in_min)/(in_max - in_min)*(out_max - out_min)+out_min;
}

void clearLine(int x,int y,int textSize=1){
  int h = 8*textSize;
  display.fillRect(x,y,SCREEN_WIDTH,h,SSD1306_BLACK);
}

void printEnvData(String line1, String line2, int textsize=1,int x=0,int y=0){
  display.setTextSize(textsize);
  display.setTextColor(SSD1306_WHITE);
  clearLine(x,y,textsize);
  display.setCursor(x,y);
  display.print(line1);
  clearLine(x,y+8*textsize,textsize);
  display.setCursor(x,y+8*textsize);
  display.print(line2);
  display.display();
}

void drawVolumeBar(int volume,int x=0,int y=48,int width=100,int height=10){
  display.fillRect(x,y,width,height,SSD1306_BLACK);
  display.drawRect(x,y,width,height,SSD1306_WHITE);
  int w = map(volume,0,100,0,width);
  display.fillRect(x,y,w,height,SSD1306_WHITE);
  display.display();
}
//─────────────────────────────────────────────────────────────────────────────

// Parse the incoming date-time string
bool parseDateTime(const String& s){
  // Expect “Sun May 18 17:21:31 2025”
  if (s.length()<20) return false;
  s.substring(0,3).toCharArray(dayOfWeek,4);
  s.substring(4,7).toCharArray(monthStr,4);
  day    = s.substring(8,10).toInt();
  hour   = s.substring(11,13).toInt();
  minute = s.substring(14,16).toInt();
  second = s.substring(17,19).toInt();
  year   = s.substring(20).toInt();
  return true;
}

// Tick the clock by one second
void tickTime(){
  second++;
  if (second==60){ second=0; minute++; 
    if (minute==60){ minute=0; hour++;
      if (hour==24){ hour=0; day++; /*no month rollover*/ } } }
}

void refreshDisplay(int pot){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  char dateBuffer[20];
  char timeBuffer[10];
  sprintf(dateBuffer, "%s %s %02d %04d", dayOfWeek, monthStr, day, year);
  display.setCursor(0,0);
  display.print(dateBuffer);

  sprintf(timeBuffer, "%02d:%02d:%02d", hour, minute, second);
  display.setCursor(0,10);
  display.print(timeBuffer);

  display.setCursor(0,20);
  display.print("Vol: ");
  display.print(pot);
  display.print("%");

  int barW = map(pot, 0, 100, 0, SCREEN_WIDTH - 20);
  display.fillRect(0,30,barW,8,SSD1306_WHITE);
  display.drawRect(0,30,SCREEN_WIDTH - 20,8,SSD1306_WHITE);

  display.setCursor(0,45);
  display.print("T:");
  display.print(temperature, 1);
  display.print("C");
  display.setCursor(0,55);
  display.print("H:");
  display.print(humidity, 1);
  display.print("%");

  display.display();
}



//─────────────────────────────────────────────────────────────────────────────
void setup(){
  Serial.begin(9600);
  while(!Serial);  // wait for USB Serial
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)){
    Serial.println("OLED init failed"); while(1);
  }
  display.clearDisplay(); display.display();
  pinMode(potPin, INPUT);

  // Ask PC for the current date/time
  Serial.println("TIME?");
  lastTimeSync = millis();
  lastDisplayRefresh = millis();
}

void loop(){
  unsigned long now = millis();

  // 1) Read incoming time sync
  if (Serial.available()){
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (parseDateTime(line)){
      Serial.println("Time OK: " + line);
    } else {
      Serial.println("Bad time format: " + line);
    }
  }

  // 2) Every 5 minutes, request a fresh sync
  if (now - lastTimeSync >= TIME_SYNC_INTERVAL){
    Serial.println("TIME?");
    lastTimeSync = now;
  }

  // 3) Poll DHT11 every 3 s
  if (now - lastSensorRead >= SENSOR_INTERVAL){
    float t, h;
    if (dht_sensor.measure(&t, &h)){
      temperature = t;
      humidity    = h;
    }
    lastSensorRead = now;
  }

  // 4) Every second, tick & update display
  if (now - lastDisplayRefresh >= ONE_SECOND){
    tickTime();

    // Read pot value here
    int pot = (int)mapValue(analogRead(potPin), 0, 1023, 0, 100);
    Serial.println(pot);
    refreshDisplay(pot);
    lastDisplayRefresh = now;
  }
}