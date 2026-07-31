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

AdafruitIO_Feed *connectionStatus = io.feed("connection-status");
AdafruitIO_Feed *temperatureFeed = io.feed("temperature");
AdafruitIO_Feed *bufferCount = io.feed("buffer-count");
AdafruitIO_Feed *systemMode = io.feed("system-mode");

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

float offlineBuffer[20];
int bufferIndex = 0;

unsigned long previousMillis = 0;
const unsigned long interval = 5000;

int mode = 0;

// Callback

void handleMode(AdafruitIO_Data *data) {
  mode = data->toInt();
}

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

  systemMode->onMessage(handleMode);

  Serial.println("Connecting to Adafruit IO...");

  io.connect();

  while (io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.println("Connected!");

  systemMode->get();
}

// Loop

void loop() {

  io.run();

  if (millis() - previousMillis >= interval) {

    previousMillis = millis();

    float temp = dht.readTemperature();

    if (isnan(temp)) {
      temp = 25.0;
    }

    display.clearDisplay();
    display.setTextSize(1);

    // ================= ONLINE =================

    if (mode == 0) {

      connectionStatus->save("ONLINE");
      temperatureFeed->save(temp);

      if (bufferIndex > 0) {

        for (int i = 0; i < bufferIndex; i++) {
          temperatureFeed->save(offlineBuffer[i]);
          delay(300);
        }

        bufferIndex = 0;
      }

      bufferCount->save(bufferIndex);

      digitalWrite(LED_PIN, HIGH);

      tone(BUZZER_PIN, 1000);
      delay(100);
      noTone(BUZZER_PIN);

      display.setCursor(0,0);
      display.println("STATUS: ONLINE");

      display.print("Temp: ");
      display.print(temp,1);
      display.println(" C");

      display.print("Buffer: ");
      display.println(bufferIndex);

      Serial.println("ONLINE");
    }

    // ================= DEGRADED =================

    else if (mode == 1) {

      connectionStatus->save("DEGRADED");
      temperatureFeed->save(temp);
      bufferCount->save(bufferIndex);

      digitalWrite(LED_PIN, HIGH);

      tone(BUZZER_PIN, 800);
      delay(200);
      noTone(BUZZER_PIN);

      display.setCursor(0,0);
      display.println("STATUS: DEGRADED");

      display.print("Temp: ");
      display.print(temp,1);
      display.println(" C");

      display.println("LIMITED SYNC");

      display.print("Buffer: ");
      display.println(bufferIndex);

      Serial.println("DEGRADED");
    }

    // ================= OFFLINE =================

    else {

      if (bufferIndex < 20) {
        offlineBuffer[bufferIndex++] = temp;
      }

      connectionStatus->save("OFFLINE");
      bufferCount->save(bufferIndex);

      digitalWrite(LED_PIN, LOW);

      tone(BUZZER_PIN, 500);
      delay(500);
      noTone(BUZZER_PIN);

      display.setCursor(0,0);
      display.println("STATUS: OFFLINE");

      display.println("LOGGING");
      display.println("OFFLINE");

      display.print("Buffer: ");
      display.println(bufferIndex);

      Serial.println("OFFLINE");
    }

    display.display();
  }
}
