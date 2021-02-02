#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "chbsem.h"

BSEMAPHORE_DECL(smph, 0);

static const uint8_t arduino_address = 0x04; //arduino address
static const uint8_t pcf_address = 0x27;     //pcf address

int aux_counter = 0;
int temperature = 0;
int humidity = 0;
int distance = 0;
int stackLineTemp[64][4];
int stackLineHum[64][4];
int screenToShow = 0;
int lcdCounter = 0;
int needsClear = 0;
int firstEnter = 1;
uint8_t pinOut[1] = {0b11111111};
uint8_t ledLevel = 0;
uint8_t lastLedLevel = 0;

// display funcs
int screenNeedsRefresh = 0;
void lcdPrintf(int x, int y, char text[], int value);
void drawLine(int x1, int y1, int x2, int y2);
void drawBox(int x1, int y1, int x2, int y2);
void clearScreen();
void drawGraphLineTemp();
void drawGraphLineHum();
void drawStructure();

// data funcs
void stackHandler();
int roundNo(float num);
int handleDistance(int value);
uint8_t handleMeasure(int handler);

void drawGraphLineTemp()
{
  int value = temperature;

  if (value > 38)
    value = 38;

  // values
  lcdPrintf(105, 47, "%d", temperature);
  lcdPrintf(105, 38, "%d", humidity);

  if (firstEnter == 1)
  {
    // title
    lcdPrintf(25, 61, "Temperature", 0);
    lcdPrintf(4, 61, "C", 0);

    // legend
    lcdPrintf(7, 22, "%u", 8);
    lcdPrintf(1, 32, "%u", 18);
    lcdPrintf(1, 42, "%u", 29);
    lcdPrintf(1, 52, "%u", 38);

    // 32/38 pixels alçada, i
    //int startX = 18;
    //int startY = 14;

    int i = 0;
    for (i = 0; i < aux_counter; i++) // draw all previous lines
    {
      drawLine(stackLineTemp[i][0],
               stackLineTemp[i][1],
               stackLineTemp[i][2],
               stackLineTemp[i][3]);
    }
    firstEnter = 0;
  }
  drawLine(stackLineTemp[aux_counter - 1][2],
           stackLineTemp[aux_counter - 1][3],
           stackLineTemp[aux_counter - 1][2] + 1,
           14 + value); // draw current line
}

void drawGraphLineHum()
{
  int value = humidity;

  if (value > 38)
    value = 38;

  // values
  lcdPrintf(105, 47, "%d", temperature);
  lcdPrintf(105, 38, "%d", humidity);

  if (firstEnter == 1)
  {
    // title
    lcdPrintf(25, 61, "Humidity", 0);
    lcdPrintf(4, 61, "%%", 0);

    // legend
    lcdPrintf(7, 22, "%u", 19);
    lcdPrintf(1, 32, "%u", 38);
    lcdPrintf(1, 42, "%u", 57);
    lcdPrintf(1, 52, "%u", 76);

    int i = 0;

    for (i = 0; i < aux_counter; i++)
    {
      drawLine(stackLineHum[i][0],
               stackLineHum[i][1],
               stackLineHum[i][2],
               stackLineHum[i][3]); // draw all previous lines
    }
    firstEnter = 0;
  }
  drawLine(stackLineHum[aux_counter - 1][2],
           stackLineHum[aux_counter - 1][3],
           stackLineHum[aux_counter - 1][2] + 1,
           14 + roundNo(value / 2)); // draw current line
}

// function to control data storing and LCD functionality
void stackHandler()
{
  if (aux_counter == 63)
    aux_counter = 0;
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
      stackLineHum[aux_counter][3] = 14 + 38;
    else
      stackLineHum[aux_counter][3] = 14 + roundNo(humidity / 2);
  }
  lcdCounter += 1;
  aux_counter += 1;
  screenNeedsRefresh = 1;

  // function to control when the graphics of LCD is going to change
  if (lcdCounter == 4)
  {
    if (screenToShow == 1)
      screenToShow = 0;
    else if (screenToShow == 0)
      screenToShow = 1;

    needsClear = 1;
    firstEnter = 1;
    lcdCounter = 0;
  }
}

void clearScreen()
{
  //clears the screen, you will use this a lot!
  sdPut(&SD1, (uint8_t)0x7C);
  sdPut(&SD1, (uint8_t)0);

  drawStructure();
  needsClear = 0;
}

void drawStructure()
{
  // info
  lcdPrintf(92, 47, "T:", 0);
  lcdPrintf(92, 38, "H:", 0);

  //
  lcdPrintf(10, 11, "%u", 0);
  lcdPrintf(118, 47, "C", 0);
  lcdPrintf(118, 38, "%%", 0);
  //mainframe
  drawLine(17, 13, 17, 52);
  drawLine(18, 13, 87, 13);

  //
  drawLine(14, 52, 17, 52);
  drawLine(14, 42, 17, 42);
  drawLine(14, 32, 17, 32);
  drawLine(14, 22, 17, 22);
  //
  drawLine(30, 12, 30, 10);
  drawLine(59, 12, 59, 10);
  drawLine(87, 12, 87, 10);
  //
  drawBox(90, 49, 125, 29);
  // legend
  lcdPrintf(30, 8, "%u", 8);
  lcdPrintf(59, 8, "%u", 16);
  lcdPrintf(87, 8, "%u", 24);
  lcdPrintf(101, 8, "h", 0);
  //
}

//	Prints a text in the LCD screen
void drawLine(int x1, int y1, int x2, int y2)
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

void drawBox(int x1, int y1, int x2, int y2)
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
  if (value >= 80)
  {
    return 1;
  }
  else if (value >= 70 && value < 80)
  {
    return 2;
  }
  else if (value >= 60 && value < 70)
  {
    return 3;
  }
  else if (value >= 50 && value < 60)
  {
    return 4;
  }
  else if (value >= 40 && value < 50)
  {
    return 5;
  }
  else if (value >= 30 && value < 40)
  {
    return 6;
  }
  else if (value >= 20 && value < 30)
  {
    return 7;
  }
  else if (value >= 10 && value < 20)
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

static WORKING_AREA(waThread_LCD, 512);
static msg_t Thread_LCD(void *p)
{
  (void)p;
  chRegSetThreadName("SerialPrint");
  drawStructure();

  while (TRUE)
  {
    if (screenNeedsRefresh == 1)
    {
      if (needsClear == 1)
        clearScreen();

      if (screenToShow == 0)
        drawGraphLineTemp();
      else if (screenToShow == 1)
        drawGraphLineHum();

      screenNeedsRefresh = 0;
    }
    chThdSleepMilliseconds(2000);
  }
  return 0;
}

static WORKING_AREA(waThread_Arduino, 512);
static msg_t Thread_Arduino(void *p)
{
  (void)p;
  chRegSetThreadName("I2cAcquiring");

  uint8_t result[] = {0, 0, 0};
  msg_t status;

  // Some time to allow slaves initialization
  chThdSleepMilliseconds(3000);

  while (TRUE)
  {
    // Request values
    status = chBSemWait(&smph);

    if (status == 0)
    {
      msg_t i2cMsg = i2cMasterReceiveTimeout(
          &I2C0, arduino_address, result, 3, MS2ST(1000));

      if (i2cMsg == 0x00)
      {
        if (result[0] != 6 && result[0] <= 60) //treat abnormal values which the temperature sensor gives sometimes
          temperature = result[0];

        humidity = result[1];
        distance = result[2];

        stackHandler();

        if (distance < 10)
          ledLevel = 1;
        else
          ledLevel = handleDistance(distance);
      }

      chBSemSignal(&smph);
    }
    chThdSleepMilliseconds(6000);
  }
  return 0;
}

//	PCF Thread

static WORKING_AREA(waThread_PCF, 1024);
static msg_t Thread_PCF(void *p)
{
  (void)p;
  chRegSetThreadName("PCF");

  msg_t status;

  chThdSleepMilliseconds(4000);

  while (TRUE)
  {
    if (ledLevel != lastLedLevel) //checks if the led level needs to be changed
    {
      status = chBSemWait(&smph);

      if (status == 0)
      {
        pinOut[0] = (uint8_t)handleMeasure(ledLevel);

        i2cMasterTransmitTimeout(&I2C0, pcf_address, pinOut,
                                 sizeof(pinOut), NULL, 0, MS2ST(2000));
        chThdSleepMilliseconds(10);

        lastLedLevel = ledLevel;

        chBSemSignal(&smph);
      }
    }
    chThdSleepMilliseconds(3000);
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

  chThdSleepMilliseconds(1000);

  // i2c Arduino Thread
  chThdCreateStatic(waThread_Arduino, sizeof(waThread_Arduino), NORMALPRIO, Thread_Arduino, NULL);

  // LCD Threat
  chThdCreateStatic(waThread_LCD, sizeof(waThread_LCD), NORMALPRIO, Thread_LCD, NULL);

  // PCF Thread
  chThdCreateStatic(waThread_PCF, sizeof(waThread_PCF), NORMALPRIO, Thread_PCF, NULL);

  // Blocks until finish
  chThdWait(chThdSelf());

  return 0;
}