#include "AdafruitIO_WiFi.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

// Adafruit IO Credentials

#define IO_USERNAME "username"
#define IO_KEY "key"

// Adafruit IO Connection

AdafruitIO_WiFi io(
  IO_USERNAME,
  IO_KEY,
  "Wokwi-GUEST",
  ""
);

// Feeds

AdafruitIO_Feed *samplingRate = io.feed("sampling-rate");
AdafruitIO_Feed *temperatureFeed = io.feed("temperature");
AdafruitIO_Feed *systemStatus = io.feed("system-status");

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

#define LED_PIN 2
#define BUZZER_PIN 4

// Variables

int sampleTime = 5;
unsigned long previousMillis = 0;

// Function

void handleSampling(AdafruitIO_Data *data);

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

  samplingRate->onMessage(handleSampling);

  Serial.println("Connecting to Adafruit IO...");

  io.connect();

  while (io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.println("Connected!");

  samplingRate->get();
}

// Loop

void loop() {

  io.run();

  if (millis() - previousMillis >= sampleTime * 1000) {

    previousMillis = millis();

    float temp = dht.readTemperature();

    if (isnan(temp)) {
      temp = 25.0;
    }

    temperatureFeed->save(temp);

    String mode;

    if (sampleTime <= 3) {

      mode = "FAST MODE";

      digitalWrite(LED_PIN, HIGH);

      tone(BUZZER_PIN, 1000);
      delay(200);
      noTone(BUZZER_PIN);

    }

    else if (sampleTime <= 7) {

      mode = "NORMAL MODE";

      digitalWrite(LED_PIN, LOW);

    }

    else {

      mode = "POWER SAVE MODE";

      digitalWrite(LED_PIN, LOW);

    }

    systemStatus->save(mode);

    // OLED

    display.clearDisplay();

    display.setTextSize(1);

    display.setCursor(0, 0);
    display.print("Temp: ");
    display.print(temp, 1);
    display.println(" C");

    display.setCursor(0, 20);
    display.print("Rate: ");
    display.print(sampleTime);
    display.println(" sec");

    display.setCursor(0, 40);
    display.print(mode);

    display.display();

    // Serial Monitor

    Serial.print("Temperature = ");
    Serial.print(temp);
    Serial.println(" C");

    Serial.print("Sampling Rate = ");
    Serial.print(sampleTime);
    Serial.println(" sec");

    Serial.print("Status = ");
    Serial.println(mode);
  }
}

// Callback

void handleSampling(AdafruitIO_Data *data) {

  sampleTime = data->toInt();

  if (sampleTime < 1)
    sampleTime = 1;

  if (sampleTime > 10)
    sampleTime = 10;

  Serial.print("New Sampling Rate: ");
  Serial.println(sampleTime);
}
