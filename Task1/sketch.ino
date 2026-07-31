#include "AdafruitIO_WiFi.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

//==================== Adafruit IO ====================

#define IO_USERNAME "username"
#define IO_KEY "key"

AdafruitIO_WiFi io(
  IO_USERNAME,
  IO_KEY,
  "Wokwi-GUEST",
  ""
);

//==================== Feeds ====================

AdafruitIO_Feed *bodyTempFeed = io.feed("body-temp");
AdafruitIO_Feed *roomTempFeed = io.feed("room-temp");
AdafruitIO_Feed *heartRateFeed = io.feed("heart-rate");
AdafruitIO_Feed *systemStatusFeed = io.feed("system-status");

//==================== OLED ====================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);

//==================== DHT22 ====================

#define DHTPIN 15
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

//==================== Pins ====================

#define POT_PIN 34
#define LED_PIN 2
#define BUZZER_PIN 4

//==================== Sensor Structure ====================

typedef struct {

  float bodyTemp;
  float roomTemp;
  int heartRate;

} SensorData;

//==================== FreeRTOS ====================

QueueHandle_t sensorQueue;
SemaphoreHandle_t oledMutex;

TaskHandle_t sensorTaskHandle;
TaskHandle_t environmentTaskHandle;
TaskHandle_t mqttTaskHandle;
TaskHandle_t alertTaskHandle;
TaskHandle_t displayTaskHandle;

//==================== Global Variables ====================

SensorData currentData;

String systemStatus = "NORMAL";

bool alertState = false;

//==================== Function Prototypes ====================

void SensorTask(void *pvParameters);
void EnvironmentTask(void *pvParameters);
void MQTTTask(void *pvParameters);
void AlertTask(void *pvParameters);
void DisplayTask(void *pvParameters);

//==================== Setup ====================

void setup() {

  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);

  dht.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {

    Serial.println("OLED Failed");

    while (1);

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

  // Queue (latest sensor reading)

  sensorQueue = xQueueCreate(1, sizeof(SensorData));

  // OLED Mutex

  oledMutex = xSemaphoreCreateMutex();

  //==================== Create FreeRTOS Tasks ====================

  xTaskCreatePinnedToCore(
    SensorTask,
    "Sensor Task",
    4096,
    NULL,
    2,
    &sensorTaskHandle,
    1
  );

  xTaskCreatePinnedToCore(
    EnvironmentTask,
    "Environment Task",
    4096,
    NULL,
    2,
    &environmentTaskHandle,
    1
  );

  xTaskCreatePinnedToCore(
    MQTTTask,
    "MQTT Task",
    4096,
    NULL,
    1,
    &mqttTaskHandle,
    0
  );

  xTaskCreatePinnedToCore(
    AlertTask,
    "Alert Task",
    2048,
    NULL,
    1,
    &alertTaskHandle,
    0
  );

  xTaskCreatePinnedToCore(
    DisplayTask,
    "Display Task",
    4096,
    NULL,
    1,
    &displayTaskHandle,
    1
  );

}


//==================== Sensor Task ====================

void SensorTask(void *pvParameters) {

  SensorData data;

  while (true) {

    float temp = dht.readTemperature();

    if (isnan(temp)) {
      temp = 25.0;
    }

    data.bodyTemp = temp;

    // Simulated Room Temperature
    data.roomTemp = temp - 2;

    // Simulated Heart Rate using Potentiometer
    int potValue = analogRead(POT_PIN);

    data.heartRate = map(potValue, 0, 4095, 60, 180);

    // Save latest values
    currentData = data;

    // Send latest sensor data to Queue
    xQueueOverwrite(sensorQueue, &data);

    Serial.print("Body Temp: ");
    Serial.print(data.bodyTemp);

    Serial.print("  Room Temp: ");
    Serial.print(data.roomTemp);

    Serial.print("  Heart Rate: ");
    Serial.println(data.heartRate);

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}
//==================== Environment Task ====================

void EnvironmentTask(void *pvParameters) {

  SensorData data;

  while (true) {

    if (xQueueReceive(sensorQueue, &data, pdMS_TO_TICKS(100)) == pdTRUE) {

      if (data.bodyTemp >= 38.0 || data.heartRate >= 120) {

        systemStatus = "ALERT";
        alertState = true;

      }
      else {

        systemStatus = "NORMAL";
        alertState = false;

      }

      currentData = data;
    }

    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

//==================== MQTT Task ====================

void MQTTTask(void *pvParameters) {

  while (true) {

    io.run();

    if (io.status() == AIO_CONNECTED) {

      bodyTempFeed->save(currentData.bodyTemp);
      roomTempFeed->save(currentData.roomTemp);
      heartRateFeed->save(currentData.heartRate);
      systemStatusFeed->save(systemStatus);

      Serial.println("Data Uploaded to Adafruit IO");

    }

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}
//==================== Alert Task ====================

void AlertTask(void *pvParameters) {

  while (true) {

    if (alertState) {

      digitalWrite(LED_PIN, HIGH);

      tone(BUZZER_PIN, 1000);
      vTaskDelay(pdMS_TO_TICKS(300));

      noTone(BUZZER_PIN);
      vTaskDelay(pdMS_TO_TICKS(300));

    }
    else {

      digitalWrite(LED_PIN, LOW);
      noTone(BUZZER_PIN);

      vTaskDelay(pdMS_TO_TICKS(500));

    }

  }

}

//==================== Display Task ====================

void DisplayTask(void *pvParameters) {

  while (true) {

    if (xSemaphoreTake(oledMutex, portMAX_DELAY) == pdTRUE) {

      display.clearDisplay();

      display.setTextSize(1);
      display.setCursor(0,0);

      display.println("FreeRTOS Monitor");
      display.println();

      display.print("Body : ");
      display.print(currentData.bodyTemp,1);
      display.println(" C");

      display.print("Room : ");
      display.print(currentData.roomTemp,1);
      display.println(" C");

      display.print("Heart: ");
      display.print(currentData.heartRate);
      display.println(" BPM");

      display.println();

      display.print("Status: ");
      display.println(systemStatus);

      display.display();

      xSemaphoreGive(oledMutex);

    }

    vTaskDelay(pdMS_TO_TICKS(1000));

  }

}

//==================== Loop ====================

void loop() {

  io.run();

  delay(10);

}
