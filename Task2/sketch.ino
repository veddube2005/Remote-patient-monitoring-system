#include "AdafruitIO_WiFi.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

// Adafruit IO Credentials

#define IO_USERNAME "user"
#define IO_KEY "key"

// Adafruit IO Connection

AdafruitIO_WiFi io(
  IO_USERNAME,
  IO_KEY,
  "Wokwi-GUEST",
  ""
);

// Feeds

AdafruitIO_Feed *heartRate = io.feed("heart-rate");
AdafruitIO_Feed *spo2 = io.feed("spo2");
AdafruitIO_Feed *bodyTemp = io.feed("body-temp");

AdafruitIO_Feed *roomTemp = io.feed("room-temp");
AdafruitIO_Feed *oxygenLevel = io.feed("oxygen-level");
AdafruitIO_Feed *aqiFeed = io.feed("aqi");

// OLED

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);

// DHT22

#define DHTPIN 15
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

// Pins

#define POT_PIN 34
#define LED_PIN 2
#define BUZZER_PIN 4

// Setup

void setup() {

  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  dht.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED Failed");
    while (true);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  Serial.println("Connecting to Adafruit IO...");

  io.connect();

  while (io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.println("Connected!");
}

// Loop

void loop()
 {

  io.run();

  float temp = dht.readTemperature();

  if (isnan(temp)) {
    temp = 25.0;
  }

  int potValue = analogRead(POT_PIN);

  int oxygen = map(potValue, 0, 4095, 80, 100);
  int aqi = map(potValue, 0, 4095, 500, 0);

  int heart = map(potValue, 0, 4095, 60, 120);
  int spo2Value = map(potValue, 0, 4095, 90, 100);

  // Publish to Adafruit IO

  heartRate->save(heart);
  spo2->save(spo2Value);
  bodyTemp->save(temp);

  roomTemp->save(temp);
  oxygenLevel->save(oxygen);
  aqiFeed->save(aqi);

  // Alerts

  bool alert = false;

  if (temp > 38) alert = true;
  if (oxygen < 85) alert = true;
  if (aqi > 300) alert = true;

  if (alert) {

    digitalWrite(LED_PIN, HIGH);

    tone(BUZZER_PIN, 1000);
    delay(200);
    noTone(BUZZER_PIN);

  } else {

    digitalWrite(LED_PIN, LOW);
  }

  // OLED Display

  display.clearDisplay();

  display.setTextSize(1);

  display.setCursor(0, 0);
  display.print("HR:");
  display.print(heart);

  display.setCursor(64, 0);
  display.print("SpO2:");
  display.print(spo2Value);

  display.setCursor(0, 16);
  display.print("Temp:");
  display.print(temp, 1);
  display.print("C");

  display.setCursor(0, 32);
  display.print("O2:");
  display.print(oxygen);
  display.print("%");

  display.setCursor(0, 48);
  display.print("AQI:");
  display.print(aqi);

  display.display();

  // Serial Monitor

  Serial.print("HR=");
  Serial.print(heart);

  Serial.print(" BPM | SpO2=");
  Serial.print(spo2Value);

  Serial.print("% | Temp=");
  Serial.print(temp);

  Serial.print("C | O2=");
  Serial.print(oxygen);

  Serial.print("% | AQI=");
  Serial.println(aqi);

  delay(5000);
}
