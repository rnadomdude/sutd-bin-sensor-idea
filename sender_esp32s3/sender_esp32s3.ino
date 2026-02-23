#include <Wire.h>
#include <VL53L0X.h>
#include <RTClib.h>
#include <esp_now.h>
#include <WiFi.h>

// PUT YOUR TRUE RECEIVER MAC ADDRESS HERE
uint8_t broadcastAddress[] = {0xE0, 0x72, 0xA1, 0xE7, 0x87, 0x88}; //E0:72:A1:E7:87:88

VL53L0X sensor;
RTC_DS3231 rtc;

typedef struct struct_message {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint16_t distance;
  bool timeout;
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

void setup() {
  Serial.begin(115200);
  
  // --- SAFETY WINDOW ---
  // Wait for 5 seconds so the USB port stays active and Windows can recognize it!
  // This gives you time to upload new code before it goes to sleep.
  delay(5000); 
  Serial.println("\n--- ESP32 WOKE UP ---");

  Wire.begin(12,13);

  // --- 1. SETUP SENSORS ---
  Serial.print("Initializing VL53L0X... ");
  sensor.setTimeout(500);
  if (!sensor.init()) {
    Serial.println("FAILED!");
    Serial.println("Going to sleep for 10s and trying again.");
    esp_sleep_enable_timer_wakeup(10 * 1000000ULL); 
    esp_deep_sleep_start();
  }
  Serial.println("OK!");

  Serial.print("Initializing RTC... ");
  if (!rtc.begin()) {
    Serial.println("FAILED!");
    esp_sleep_enable_timer_wakeup(10 * 1000000ULL);
    esp_deep_sleep_start();
  }
  Serial.println("OK!");

  // --- 2. SETUP ESP-NOW ---
  Serial.print("Initializing ESP-NOW... ");
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("FAILED!");
    esp_sleep_enable_timer_wakeup(10 * 1000000ULL);
    esp_deep_sleep_start();
  }
  Serial.println("OK!");

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  // --- 3. GATHER DATA ---
  Serial.print("Reading sensors... ");
  DateTime now = rtc.now();
  
  myData.year = now.year();
  myData.month = now.month();
  myData.day = now.day();
  myData.hour = now.hour();
  myData.minute = now.minute();
  myData.second = now.second();
  
  myData.distance = sensor.readRangeSingleMillimeters();
  myData.timeout = sensor.timeoutOccurred();
  Serial.println("Done.");

  // --- 4. SEND DATA ---
  Serial.print("Transmitting packet... ");
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  if (result == ESP_OK) {
    Serial.println("Success!");
  } else {
    Serial.println("Failed to send.");
  }

  // Give the Wi-Fi radio a fraction of a second to finish transmitting
  delay(100); 

  // --- 5. GO TO DEEP SLEEP ---
  Serial.println("Task complete. Going to deep sleep for 10 seconds...");
  Serial.println("-----------------------\n");
  
  esp_sleep_enable_timer_wakeup(10 * 1000000ULL); 
  esp_deep_sleep_start(); 
}

void loop() {
  // This will never run because deep sleep resets the board back to setup()!
}