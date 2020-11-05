#include <SoftwareSerial.h>
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

boolean newData = false;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  mySerial.begin(115200);
  delay(5000);
}

// Enter data in this style <HelloWorld, 12, 24.7>

void loop() {
    recvWithStartEndMarkers();
    if (newData == true) {
        strcpy(tempChars, receivedChars);
            // this temporary copy is necessary to protect the original data
            //   because strtok() used in parseData() replaces the commas with \0
        parseData();
        showParsedData();
        newData = false;
    }
}

//============

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

//============

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

}

//============

void showParsedData() {
    Serial.print("Message: ");
    Serial.println(message);
    Serial.print("Distance (HC-SR04): ");
    Serial.println(floatDistance);
    Serial.print("Time (HC-SR04): ");
    Serial.println(floatTime);
    Serial.print("Temperature (DHT11): ");
    Serial.println(floatTemp);
    Serial.print("Humidity (DHT11): ");
    Serial.println(floatHum);
}
