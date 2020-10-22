#include <ESP8266WiFi.h>
#include <espnow.h>

// Structure to receive data
// Must match the sender structure
typedef struct struct_message {
    int id;
    float x;
    float y;
} struct_message;

// Create a struct_message called myData
struct_message myData;

// Create a structure to hold the readings from each board
struct_message board1;
struct_message board2;

// Create an array with all the structures
struct_message boardsStruct[2] = {board1, board2};

unsigned long lastTime = 0;
unsigned long timerDelay = 10000;

float board1Distance = 0.0;
float board1Time = 0.0;
float board2Temp = 0.0;
float board2Hum = 0.0;

// Callback function that will be executed when data is received
void OnDataRecv(uint8_t * mac_addr, uint8_t *incomingData, uint8_t len) {
  char macStr[18];
  //Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  //Serial.println(macStr);
  memcpy(&myData, incomingData, sizeof(myData));
  //Serial.printf("Board ID %u: %u bytes\n", myData.id, len);
  // Update the structures with the new incoming data
  boardsStruct[myData.id-1].x = myData.x;
  boardsStruct[myData.id-1].y = myData.y;
  boardsStruct[myData.id-1].id = myData.id;
  //Serial.printf("x value: %f \n", boardsStruct[myData.id-1].x);
  //Serial.printf("y value: %f \n", boardsStruct[myData.id-1].y);
  //Serial.println();
}
 
void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);
}

void loop(){
   // loop
  if ((millis() - lastTime) > timerDelay) {
    // Access the variables for each board

    board1Distance = boardsStruct[0].x;
    board1Time = boardsStruct[0].y;
    
    board2Temp = boardsStruct[1].x;
    board2Hum = boardsStruct[1].y;
    
    // Send the information to the arduino.
    Serial.print("<Data,");
    Serial.print(board1Distance);
    Serial.print(",");
    Serial.print(board1Time);
    Serial.print(",");
    Serial.print(board2Temp);
    Serial.print(",");
    Serial.print(board2Hum);
    Serial.print(">");

    lastTime = millis();
  }
}
