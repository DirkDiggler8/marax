//Includes
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>

//Defines
#define SCREEN_WIDTH    128     //Width in px 
#define SCREEN_HEIGHT   64      // Height in px
#define OLED_RESET      -1
#define SCREEN_ADDRESS  0x3C    // or 0x3D Check datasheet or Oled Display
#define BUFFER_SIZE     32

//Pins
int d5 = 5; //orange PIN 4 Mara TX to Arduino RX D5
int d6 = 6; //black  PIN 3 Mara RX to Arduino TX D6

//Internals
long lastPumpOnMillis = 0;
int seconds = 0;
int lastTimer = 0;
long serialTimeout = 0;
char buffer[BUFFER_SIZE];
int index = 0;
int isMaraOff = 0;
long lastToggleTime = 0;
int HeatDisplayToggle = 0;

const int Sim = 0;

//Mara Data
String maraData[7];

//Instances
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
SoftwareSerial mySerial(d5, d6);

void SetSim()
{
  if (Sim == 1)
  {
    //C1.06,116,124,093,0840,1,0
    maraData[0] = String("C1.06");
    maraData[1] = String("116");
    maraData[2] = String("124");
    maraData[3] = String("093");
    maraData[4] = String("0840");
    maraData[5] = String("1");
    maraData[6] = String("0");
  }
}

void setup()
{
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.clearDisplay();
  display.display();
  Serial.begin(9600);
  mySerial.begin(9600);
  memset(buffer, 0, BUFFER_SIZE);
  mySerial.write(0x11);
}

void getMaraData()
{
  /*
    Example Data: C1.06,116,124,093,0840,1,0\n every ~400-500ms
    Length: 26
    [Pos] [Data] [Describtion]
    0)      C     Coffee Mode (C) or SteamMode (V)
    -        1.06  Software Version
    1)      116   current steam temperature (Celsisus)
    2)      124   target steam temperature (Celsisus)
    3)      093   current hx temperature (Celsisus)
    4)      0840  countdown for 'boost-mode'
    5)      1     heating element on or off
    6)      0     pump on or off
  */

  while (mySerial.available())
  {
    isMaraOff = 0;
    serialTimeout = millis();
    char rcv = mySerial.read();
    if (rcv != '\n')
      buffer[index++] = rcv;
    else 
    {
      index = 0;
      Serial.println(buffer);
      char* ptr = strtok(buffer, ",");
      int idx = 0;
      while (ptr != NULL)
      {
        maraData[idx++] = String(ptr);
        ptr = strtok(NULL, ",");
      }
    }
  }
  if (millis() - serialTimeout > 6000)
  {
    isMaraOff = 1;
    SetSim();
    serialTimeout = millis();
    mySerial.write(0x11);

  }
}

void updateView()
{
  display.clearDisplay();
  display.setTextColor(WHITE);

  if (seconds == 0)
  {
    if (isMaraOff == 1)
    {
      display.setCursor(30, 30);
      display.setTextSize(4);
      display.print("OFF");
    }
    else
    {
      //HX
      display.setCursor(2, 30);
      display.setTextSize(4);
      display.print(maraData[3].toInt());
      display.setTextSize(1);
      display.print((char)247);
      display.setTextSize(1);
      display.print("C");

      //Steam
      display.setCursor(80, 30);
      display.setTextSize(2);
      display.print(maraData[1].toInt());
      display.setTextSize(1);
      display.print((char)247);
      display.setTextSize(1);
      display.print("C");

      //Draw Line
      display.drawLine(74, 0, 74, 64, WHITE);
      display.drawLine(0, 18, 128, 18, WHITE);

      //Target
      display.setCursor(80, 2);
      display.setTextSize(2);
      display.print(maraData[2].toInt());
      display.setTextSize(1);
      display.print((char)247);
      display.setTextSize(1);
      display.print("C");

      //Boiler
      display.setCursor(8, 5);
      display.setTextSize(1);
      
      if (maraData[5].toInt() == 1)
      {
        if ((millis() - lastToggleTime) > 1000)
        {
          lastToggleTime = millis();
          if (HeatDisplayToggle == 1)
          {
            HeatDisplayToggle = 0;
          }
          else
          {
            HeatDisplayToggle = 1;
          }
        }
        if (HeatDisplayToggle == 1)
        {
          display.print("Heating...");
        }
        else
        {
          display.print("");
        }
        display.fillRect(0, 0, 4, 14, WHITE);
      }
      else
      {
        display.print("");
        display.fillRect(0, 0, 4, 14, BLACK);
      }

      //Mode
      display.setTextSize(1);
      display.setCursor(80, 55);
      if (maraData[0].substring(0, 1) == "C")
      {
        display.print("Coffee");
      }
      else
      {
        display.print("Steam");
      }   
    }
  } 
  else
  {
    display.fillRect(0, 0, 124, 16, WHITE);
    display.setCursor(35, 25);
    display.setTextSize(5);
    if (seconds < 10)
    {
      display.print("0");
    }
    display.print(seconds);
  }

  display.display();
}

void loop()
{
  getMaraData();

  int pumpState = maraData[6].toInt();
  if (pumpState == 1)
  {
    if (millis() - lastPumpOnMillis >= 1000)
    {
      lastPumpOnMillis = millis();
      ++seconds;
    }
  }
  else
  {
    seconds = 0;
  }

  updateView();
}
