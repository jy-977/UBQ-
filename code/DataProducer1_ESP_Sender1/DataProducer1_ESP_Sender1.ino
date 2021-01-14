/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp-now-many-to-one-esp8266-nodemcu/

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#include <ESP8266WiFi.h>
#include <espnow.h>
//#include <Ultrasonic.h>

// REPLACE WITH RECEIVER MAC Address (ESP-01 thats connected with the Arduino)
uint8_t broadcastAddress[] = {0xA4, 0xCF, 0x12, 0xBF, 0x15, 0xEE};

// pins of the esp8266 connected to the sensor
#define trigPin 2  //GPIO2
#define echoPin 0  //GPIO0
//Ultrasonic ultrasonic(0, 2); // trigger, echo

// intialize the variables for storing the measurement
long duration; // variable for the duration of sound wave travel
long distance; // variable for the distance measurement

// Set your Board ID (ESP32 Sender #1 = BOARD_ID 1, ESP32 Sender #2 = BOARD_ID 2, etc)
#define BOARD_ID 1

// Structure to send data
// Must match the receiver structure
typedef struct struct_message {
  int id;
  float x;
  float y;
} struct_message;

// Create a struct_message called test to store variables to be sent
struct_message myData;

unsigned long lastTime = 0;
unsigned long timerDelay = 4000;

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("\r\nLast Packet Send Status: ");
  if (sendStatus == 0) {
    Serial.println("Delivery success");
  }
  else {
    Serial.println("Delivery fail");
  }
}

void setup() {
  // Init Serial Monitor
  Serial.begin(115200);

  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  // Set ESP-NOW role
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);

  // Once ESPNow is successfully init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);

}

void loop() {
  
  digitalWrite(trigPin, LOW);
  delayMicroseconds(4);
  // Sets the trigPin HIGH (ACTIVE) for 10 microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  // Calculating the distance
  distance = duration / 58.2; // Speed of sound wave divided by 2 (go and back)
  
  //distance = ultrasonic.distanceRead();

  delay(1000);

  if ((millis() - lastTime) > timerDelay) {
    // do the measurements of the sensor
    /*
      digitalWrite(trigPin, LOW);  //para generar un pulso limpio ponemos a LOW 4us
      delayMicroseconds(4);

      digitalWrite(trigPin, HIGH);  //generamos Trigger (disparo) de 10us
      delayMicroseconds(10);
      digitalWrite(trigPin, LOW);

      timeElapsed = pulseIn(echoPin, HIGH);
      distance = timeElapsed/58.3;
    */


    // Set values to send
    myData.id = BOARD_ID;
    myData.x = distance;
    myData.y = duration;

    // Send message via ESP-NOW
    esp_now_send(0, (uint8_t *) &myData, sizeof(myData));
    lastTime = millis();
  }
}
