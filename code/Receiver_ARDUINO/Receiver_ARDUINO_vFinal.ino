#include <SoftwareSerial.h>
#include <Wire.h>
#define I2C_ADDR 0x04

uint8_t data;

SoftwareSerial mySerial(2, 3); // RX, TX

// Variables to handle the data from the ESP
const byte numChars = 64;
char receivedChars[numChars];
char tempChars[numChars];        // temporary array for use when parsing

// variables to hold the parsed data
char message[numChars] = {0};
float floatTemp = 0.0;
float floatHum = 0.0;
float floatDistance = 0.0;
float floatTime = 0.0;
int sensorData[3];
volatile int lastTemp = 1;
volatile int lastHum = 1;
volatile int lastDistance = 1;

boolean newData = false;

void setup() {
  // put your setup code here, to run once:
  // Serial communication with the ESP-01
  Serial.begin(9600);
  mySerial.begin(115200);

  Wire.begin(I2C_ADDR);

  Wire.onRequest(sendData_handler);
  delay(1000);
}

// Data received from the Data Receiver ESP-01 are: <String,float, float, float, float>

void loop() {
    recvWithStartEndMarkers();
    if (newData == true) {
        strcpy(tempChars, receivedChars);
            // this temporary copy is necessary to protect the original data
            //   because strtok() used in parseData() replaces the commas with \0
        parseData();
        //showParsedData();
        newData = false;
    }
    delay(500);
}

//============

// Deals with the received data

void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;

    while (mySerial.available() > 0 && newData == false) {
        rc = mySerial.read();

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}

//============ Splits received data in 5 variables, one string as 4 floats (2 of each sensor)

void parseData() {      // split the data into its parts

    char * strtokIndx; // this is used by strtok() as an index

    strtokIndx = strtok(tempChars,",");      // get the first part - the string
    strcpy(message, strtokIndx); // copy it to

    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
    floatDistance = atof(strtokIndx);     // convert this part to a float

    strtokIndx = strtok(NULL, ",");
    floatTime = atof(strtokIndx);     // convert this part to a float
 
    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
    floatTemp = atof(strtokIndx);     // convert this part to a float

    strtokIndx = strtok(NULL, ",");
    floatHum = atof(strtokIndx);     // convert this part to a float

    if (floatTemp > 0.0 && floatTemp < 100.0) {
      lastTemp = int(floatTemp);
    }
    
    if (floatHum > 0.0 && floatHum < 150.0) {
      lastHum = int(floatHum);
    } 
    
    if (floatDistance > 0.0 && floatDistance < 1000.0) {
      lastDistance = int(floatDistance); 
    }
}

//============

// Prints variable

void showParsedData() {
    Serial.print("Message: ");
    Serial.println(message);
    Serial.print("Distance (HC-SR04): ");
    Serial.println(lastDistance);
    Serial.print("Time (HC-SR04): ");
    Serial.println(floatTime);
    Serial.print("Temperature (DHT11): ");
    Serial.println(lastTemp);
    Serial.print("Humidity (DHT11): ");
    Serial.println(lastHum);
}

void sendData_handler (){

  sensorData[0] = lastTemp;
  sensorData[1] = lastHum;
  sensorData[2] = lastDistance;
  
  for (int i=0; i<3; i++) {
      Wire.write(sensorData[i]);  //data bytes are queued in local buffer
  }
}
