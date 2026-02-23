#include <esp_now.h>
#include <WiFi.h>

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

// A flag to tell the main loop() that new data has arrived
volatile bool newDataReceived = false; 

// --- UPDATED CALLBACK ---
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  if (len == sizeof(struct_message)) {
    // Copy the data quickly and get out! Do NOT print here.
    memcpy((void*)&myData, incomingData, sizeof(myData));
    newDataReceived = true; 
  } else {
    // We can print a tiny error here, but it's risky. 
    // Best to just ignore corrupted packets quietly.
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000);
  
  Serial.println("Timestamp,Distance_mm");
  
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error,ESP-NOW_Init_Failed");
    return;
  }
  
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  // --- SAFELY PRINT DATA IN THE MAIN LOOP ---
  if (newDataReceived) {
    int distance_out = myData.timeout ? -1 : myData.distance;
    
    char csvRow[64];
    snprintf(csvRow, sizeof(csvRow), "%04d-%02d-%02d %02d:%02d:%02d,%d", 
             myData.year, myData.month, myData.day, 
             myData.hour, myData.minute, myData.second, 
             distance_out);
             
    Serial.println(csvRow);
    
    // Reset the flag so we wait for the next packet
    newDataReceived = false;
  }
}