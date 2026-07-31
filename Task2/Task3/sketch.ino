#include "AdafruitIO_WiFi.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Adafruit IO Credentials

#define IO_USERNAME "username"
#define IO_KEY      "key"

// Adafruit IO Connection

AdafruitIO_WiFi io(
  IO_USERNAME,
  IO_KEY,
  "Wokwi-GUEST",
  ""
);

// Feeds

AdafruitIO_Feed *dosageFeed = io.feed("dosage");
AdafruitIO_Feed *statusFeed = io.feed("dosage-status");

// OLED

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);

// Pins

#define LED_PIN 2
#define BUZZER_PIN 4

int dosage = 0;
String statusText = "Normal";

// Callback Function

void handleDosage(AdafruitIO_Data *data) {

  dosage = data->toInt();

  if (dosage <= 50) {

    statusText = "Normal";

    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);

  }
  else if (dosage <= 80) {

    statusText = "Warning";

    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, LOW);

  }
  else {

    statusText = "Critical";

    digitalWrite(LED_PIN, HIGH);

    tone(BUZZER_PIN, 1000);
    delay(300);
    noTone(BUZZER_PIN);
  }

  statusFeed->save(statusText);

  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("MEDICATION");

  display.setCursor(0, 20);
  display.print("Dose: ");
  display.println(dosage);

  display.setCursor(0, 40);
  display.print("Status:");
  display.println(statusText);

  display.display();

  Serial.print("Dosage: ");
  Serial.print(dosage);
  Serial.print(" | Status: ");
  Serial.println(statusText);
}

// Setup

void setup() {

  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED Failed");
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.println("Medication System");
  display.display();

  dosageFeed->onMessage(handleDosage);

  Serial.println("Connecting to Adafruit IO...");

  io.connect();

  while (io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.println("Connected!");

  dosageFeed->get();
}

// Loop

void loop() {
  io.run();
}
