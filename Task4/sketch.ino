#include "AdafruitIO_WiFi.h"
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


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
AdafruitIO_Feed *bedAngle = io.feed("bed-angle");
AdafruitIO_Feed *bedMode  = io.feed("bed-mode");


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


#define SERVO_PIN 18
#define LED_PIN 2
#define BUZZER_PIN 4

Servo bedServo;

int currentAngle = 0;
String currentMode = "Sleeping";


// Feed Callback


void handleBedAngle(AdafruitIO_Data *data) {

  currentAngle = data->toInt();

  // Move Servo
  bedServo.write(currentAngle);

  // Determine Mode
  if (currentAngle <= 20) {
    currentMode = "Sleeping";

    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }
  else if (currentAngle <= 60) {
    currentMode = "Breathing Support";

    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, LOW);
  }
  else {
    currentMode = "Emergency";

    digitalWrite(LED_PIN, HIGH);

    tone(BUZZER_PIN, 1000);
    delay(300);
    noTone(BUZZER_PIN);
  }

  // Update Feed
  bedMode->save(currentMode);

  // Update OLED
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("SMART BED");

  display.setCursor(0, 20);
  display.print("Angle: ");
  display.print(currentAngle);
  display.println((char)247);

  display.setCursor(0, 40);
  display.print("Mode:");
  display.println(currentMode);

  display.display();

  Serial.print("Angle: ");
  Serial.print(currentAngle);

  Serial.print(" | Mode: ");
  Serial.println(currentMode);
}


// Setup


void setup() {

  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  bedServo.attach(SERVO_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED Failed");
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.println("Smart Bed System");
  display.display();

  // Feed callback
  bedAngle->onMessage(handleBedAngle);

  Serial.println("Connecting to Adafruit IO...");

  io.connect();

  while (io.status() < AIO_CONNECTED) {
  Serial.print(".");
  delay(500);
}

  Serial.println();
  Serial.println("Connected!");

  bedAngle->get();
}


// Loop


void loop() {
  io.run();
}
