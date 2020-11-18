#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "chbsem.h"

//binary_semaphore_t smph;
//SEMAPHORE_DCL(smph, 0);
BSEMAPHORE_DECL(smph, 0);

#define arduino_address 0x04   //arduino address
#define pcf_address 0x27       //device address
#define pcf_address_write 0x4E // pcf address + 0 bit write

// Aux variables
int temperature;
int humidity;
int distance;
int level;
uint8_t measure;
int aux_counter = 0;
int aux_pcf = 0;
uint8_t pinOut = 0b11111111;
//uint8_t dataOut = 0xFF;
int stackLineTemp[128][4] = {0, 0, 0, 0};
int stackLineHum[128][4] = {0, 0, 0, 0};

// LCD
const int maxX = 128;
const int maxY = 64;

// display funcs
void lcdPrintf(int x, int y, char text[], int value);
void drawLine(int x1, int y1, int x2, int y2, int set);
void drawBox(int x1, int y1, int x2, int y2, int set);
void stackHandler(int value);
void drawGraphLineTemp(int value);
void drawGraphLineHum(int value);
void drawStructure();
void clearScreen();
// aux funcs
int roundNo(float num);
// pcf funcs
int handleDistance(int value);
uint8_t handleMeasure(int handler);
static void i2cled_init(uint8_t device_address, uint8_t dirmask);
static msg_t i2cled_write(uint8_t device_address,
                          uint8_t register_address,
                          uint8_t data);

// i2c functions

static msg_t i2cled_write(uint8_t device_address,
                          uint8_t register_address,
                          uint8_t data)
{
  uint8_t request[2];
  request[0] = register_address;
  request[1] = data;

  msg_t status = i2cMasterTransmit(
      &I2C0, device_address, request, 2,
      NULL, 0);
  chThdSleepMilliseconds(50);

  if (status != RDY_OK)
    //chprintf((BaseSequentialStream *)&SD1, "Error while writing to i2cled: %d\r\n", status);

    return status;
}

static void i2cled_init(uint8_t device_address, uint8_t dirmask)
{
  msg_t status = i2cled_write(device_address,
                              pcf_address_write, // direction register.
                              dirmask);
  chThdSleepMilliseconds(50);

  if (status != RDY_OK)
  //chprintf((BaseSequentialStream *)&SD1, "Error while setting direction mask: %d\r\n", status);
}

//	I2C thread to receive information from Arduino

static WORKING_AREA(waThread_I2C, 512);
static msg_t Thread_I2C(void *p)
{
  (void)p;
  chRegSetThreadName("I2cAcquiring");
  uint8_t request[] = {0};
  uint8_t result[] = {0, 0, 0};
  msg_t status;

  // Some time to allow slaves initialization
  chThdSleepMilliseconds(2000);

  while (TRUE)
  {
    // Request values

    //i2cAcquireBus(&I2C0);
    chBSemWait(&smph);

    //i2cMasterTransmitTimeout(
    //    &I2C0, arduino_address, request, 1,
    //    result, 3, MS2ST(1000));

    i2cMasterReceiveTimeout(
        &I2C0, arduino_address, result, 3, MS2ST(1000));

    chThdSleepMilliseconds(10);

    // uncomment to code
    temperature = result[0];
    humidity = result[1];
    distance = result[2];

    //i2cReleaseBus(&I2C0);

    chBSemSignal(&smph);
    chThdSleepMilliseconds(2000);
  }
  return 0;
}

//	Display in LCD thread

static WORKING_AREA(waThread_LCD, 128);
static msg_t Thread_LCD(void *p)
{
  (void)p;
  chRegSetThreadName("SerialPrint");

  //drawStructure();
  // aux variable to control which screen to show
  int screen = 0;

  while (TRUE)
  {
    //  lcdPrintf(32, 32, "Iteration: %u", iteration);
    chBSemWait(&smph);

    drawStructure();

    if (aux_counter == 0)
    {
      stackLineTemp[0][0] = 18;
      stackLineTemp[0][1] = 14 + temperature;
      stackLineTemp[0][2] = 18 + 1;
      stackLineTemp[0][3] = 14 + temperature;

      stackLineHum[0][0] = 18;
      stackLineHum[0][1] = 14 + roundNo(humidity / 2);
      stackLineHum[0][2] = 18 + 1;
      stackLineHum[0][3] = 14 + roundNo(humidity / 2);
    }
    else
    {
      stackHandler(0);

      if (screen == 0)
      {
        drawGraphLineTemp(temperature);
        if (aux_counter % 5 == 0)
        {
          screen = 1;
          clearScreen();
        }
      }
      else if (screen == 1)
      {
        drawGraphLineHum(humidity);
        if (aux_counter % 10 == 0)
        {
          screen = 0;
          clearScreen();
        }
      }
    }

    chBSemSignal(&smph);
    chThdSleepMilliseconds(2000);
  }
  return 0;
}

void stackHandler(int value)
{
  stackLineTemp[aux_counter][0] = stackLineTemp[aux_counter - 1][2];
  stackLineTemp[aux_counter][1] = stackLineTemp[aux_counter - 1][3];
  stackLineTemp[aux_counter][2] = stackLineTemp[aux_counter - 1][2] + 1;
  if (temperature > 38)
    stackLineTemp[aux_counter][3] = 14 + 38;
  else
    stackLineTemp[aux_counter][3] = 14 + temperature;

  stackLineHum[aux_counter][0] = stackLineHum[aux_counter - 1][2];
  stackLineHum[aux_counter][1] = stackLineHum[aux_counter - 1][3];
  stackLineHum[aux_counter][2] = stackLineHum[aux_counter - 1][2] + 1;
  if (humidity > 76)
    stackLineHum[aux_counter][3] = 14 + 76;
  else
    stackLineHum[aux_counter][3] = 14 + roundNo(humidity / 2);
}

void drawGraphLineTemp(int value)
{
  if (value > 38)
    value = 38;

  // title
  lcdPrintf(25, 61, "Temperature", 0);
  lcdPrintf(4, 61, "C", 0);

  // legend
  lcdPrintf(7, 22, "%u", 8);
  lcdPrintf(1, 32, "%u", 18);
  lcdPrintf(1, 42, "%u", 29);
  lcdPrintf(1, 52, "%u", 38);

  // values
  lcdPrintf(105, 47, "%u", temperature);
  lcdPrintf(105, 38, "%u", humidity);
  lcdPrintf(105, 27, "%u", aux_counter);

  // 32/38 pixels alçada, i
  //int startX = 18;
  //int startY = 14;

  int i = 0;
  for (i = 0; i < aux_counter; i++) // draw all previous lines
  {
    drawLine(stackLineTemp[i][0],
             stackLineTemp[i][1],
             stackLineTemp[i][2],
             stackLineTemp[i][3], 0);
  }
  drawLine(stackLineTemp[aux_counter - 1][2],
           stackLineTemp[aux_counter - 1][3],
           stackLineTemp[aux_counter - 1][2] + 1,
           14 + value, 0); // draw current line
}

void drawGraphLineHum(int value)
{
  if (value > 76)
    value = 76;

  // title
  lcdPrintf(25, 61, "Humidity", 0);
  lcdPrintf(4, 61, "%%", 0);

  // legend
  lcdPrintf(7, 22, "%u", 19);
  lcdPrintf(1, 32, "%u", 38);
  lcdPrintf(1, 42, "%u", 57);
  lcdPrintf(1, 52, "%u", 76);

  // values
  lcdPrintf(105, 47, "%u", temperature);
  lcdPrintf(105, 38, "%u", humidity);
  lcdPrintf(105, 27, "%u", aux_counter);

  int i = 0;

  for (i = 0; i < aux_counter; i++)
  {
    drawLine(stackLineHum[i][0],
             stackLineHum[i][1],
             stackLineHum[i][2],
             stackLineHum[i][3], 0); // draw all previous lines
  }
  drawLine(stackLineHum[aux_counter - 1][2],
           stackLineHum[aux_counter - 1][3],
           stackLineHum[aux_counter - 1][2] + 1,
           14 + roundNo(value / 2), 0); // draw current line
}

void drawStructure()
{
  // info
  lcdPrintf(92, 47, "T:", 0);
  lcdPrintf(92, 38, "H:", 0);
  lcdPrintf(92, 27, "D:", 0);
  //
  lcdPrintf(10, 11, "%u", 0);
  lcdPrintf(118, 47, "C", 0);
  lcdPrintf(118, 38, "%%", 0);
  //mainframe
  drawLine(17, 13, 17, 52, 0);
  drawLine(18, 13, 87, 13, 0);
  //
  drawLine(14, 52, 17, 52, 0);
  drawLine(14, 42, 17, 42, 0);
  drawLine(14, 32, 17, 32, 0);
  drawLine(14, 22, 17, 22, 0);
  //
  drawLine(30, 12, 30, 10, 0);
  drawLine(59, 12, 59, 10, 0);
  drawLine(87, 12, 87, 10, 0);
  //
  drawBox(90, 49, 125, 29, 0);
  // legend
  lcdPrintf(30, 8, "%u", 8);
  lcdPrintf(59, 8, "%u", 16);
  lcdPrintf(87, 8, "%u", 24);
  lcdPrintf(101, 8, "h", 0);
  //
}

//	Prints a text in the LCD screen
void drawLine(int x1, int y1, int x2, int y2, int set)
{
  //draws a line from two given points. You can set and reset just as the pixel function.
  sdPut(&SD1, (uint8_t)0x7C);
  sdPut(&SD1, (uint8_t)0x0C);
  sdPut(&SD1, (uint8_t)x1);
  sdPut(&SD1, (uint8_t)y1);
  sdPut(&SD1, (uint8_t)x2);
  sdPut(&SD1, (uint8_t)y2);
  sdPut(&SD1, (uint8_t)0x01);

  chThdSleepMilliseconds(10);
}

void drawBox(int x1, int y1, int x2, int y2, int set)
{
  //draws a box from two given points. You can set and reset just as the pixel function.
  sdPut(&SD1, (uint8_t)0x7C);
  sdPut(&SD1, (uint8_t)0x0F);
  sdPut(&SD1, (uint8_t)x1);
  sdPut(&SD1, (uint8_t)y1);
  sdPut(&SD1, (uint8_t)x2);
  sdPut(&SD1, (uint8_t)y2);
  sdPut(&SD1, (uint8_t)0x01);

  chThdSleepMilliseconds(10);
}

int roundNo(float num)
{
  return num < 0 ? num - 0.5 : num + 0.5;
}

void clearScreen()
{
  //clears the screen, you will use this a lot!
  sdPut(&SD1, (uint8_t)0x7C);
  sdPut(&SD1, (uint8_t)0);

  drawStructure();
}

void lcdPrintf(int x, int y, char text[], int value)
{

  sdPut(&SD1, (uint8_t)0x7C);
  sdPut(&SD1, (uint8_t)0x18);
  sdPut(&SD1, (uint8_t)x);
  chThdSleepMilliseconds(10);

  sdPut(&SD1, (uint8_t)0x7C);
  sdPut(&SD1, (uint8_t)0x19);
  sdPut(&SD1, (uint8_t)y);
  chThdSleepMilliseconds(10);

  chprintf((BaseSequentialStream *)&SD1, text, value);
  chThdSleepMilliseconds(10);
}

// Function to convert the distance in 8 levels
int handleDistance(int value)
{
  if (value >= 400)
  {
    return 1;
  }
  else if (value >= 350 && value < 400)
  {
    return 2;
  }
  else if (value >= 300 && value < 350)
  {
    return 3;
  }
  else if (value >= 250 && value < 300)
  {
    return 4;
  }
  else if (value >= 200 && value < 250)
  {
    return 5;
  }
  else if (value >= 150 && value < 200)
  {
    return 6;
  }
  else if (value >= 100 && value < 150)
  {
    return 7;
  }
  else if (value < 100)
  {
    return 8;
  }
  else
  {
    return 0;
  }
}

// Function which manages the received value and returns the bytes (LEDs to turn on in PCF8574)
uint8_t handleMeasure(int handler)
{
  switch (handler)
  {
  case 8:
    return 0b00001000;
  case 7:
    return 0b00000000;
  case 6:
    return 0b10000000;
  case 5:
    return 0b11000000;
  case 4:
    return 0b11100000;
  case 3:
    return 0b11110000;
  case 2:
    return 0b11110100;
  case 1:
    return 0b11110110;
  case 0:
  default:
    return 0b11110111;
    //return 0b11111111;
  }
}

//	PCF Thread

static WORKING_AREA(waThread_PCF, 128);
static msg_t Thread_PCF(void *p)
{
  (void)p;
  chRegSetThreadName("PCF");
  chThdSleepMilliseconds(2000);
  int distanceHandler = 0;

  while (TRUE)
  {
    chBSemWait(&smph);
    palSetPad(ONBOARD_LED_PORT, ONBOARD_LED_PAD);

    distanceHandler = handleDistance(distance);
    pinOut = (uint8_t)handleMeasure(distanceHandler);

    //i2cMasterTransmit(&I2C0, pcf_address_write, (uint8_t)pinOut, sizeof(pinOut), NULL, 0);
    i2cled_write(pcf_address, pcf_address_write, pinOut);

    //lcdPrintf(92, 17, "%u", pinOut);

    palClearPad(ONBOARD_LED_PORT, ONBOARD_LED_PAD);
    aux_counter += 1;
    aux_pcf += 1;
    //temperature += 3;
    //humidity += 5;

    if (aux_pcf == 8)
      aux_pcf = 0;
    if (aux_counter % 15 == 0)
    {
      temperature = 6;
      humidity = 7;
    }
    chThdSleepMilliseconds(2000);
    chBSemSignal(&smph);
  }

  return 0;
}

int main(void)
{
  halInit();
  chSysInit();

  // Initialize Serial Port
  sdStart(&SD1, NULL);

  /*
   * I2C initialization.
   */
  I2CConfig i2cConfig;
  i2cStart(&I2C0, &i2cConfig);

  //i2cMasterTransmit(&I2C0, pcf_address, 0b11111111, 1, NULL, 0);
  chThdSleepMilliseconds(2000);
  i2cled_init(pcf_address, 0x00);

  chBSemInit(&smph, 0);

  // i2c Arduino Thread
  chThdCreateStatic(waThread_I2C, sizeof(waThread_I2C),
                    NORMALPRIO, Thread_I2C, NULL);

  // LCD Threat
  chThdCreateStatic(waThread_LCD, sizeof(waThread_LCD), NORMALPRIO, Thread_LCD, NULL);

  // PCF Thread
  chThdCreateStatic(waThread_PCF, sizeof(waThread_PCF), NORMALPRIO, Thread_PCF, NULL);

  // Blocks until finish
  chThdWait(chThdSelf());

  return 0;
}
