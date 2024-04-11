/*********************************************************************
 Project: Cumin Lander
 Author: Mohit Bhoite
 Date: 10 April 2024

 Uses BLE library and code example by Adafruit.

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TimeLib.h>
#include <Adafruit_BME280.h>
#include "DSEG7_Classic_Mini_Regular_15.h"

#include <bluefruit.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>

//#include "Adafruit_SHT31.h"

// BLE Service
BLEDfu  bledfu;  // OTA DFU service
BLEDis  bledis;  // device information
BLEUart bleuart; // uart over ble
BLEBas  blebas;  // battery

#define RED_LED   D2
#define GREEN_LED D6
#define speakerPin D0
#define BUTTON D1

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels


#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_BME280 bme; // I2C

GFXcanvas1 canvas(128, 32);

char timeHour[8];
char timeMinute[8];
char timeSecond[8];

uint8_t myHour;
uint8_t myMinute;
uint8_t mySecond;
uint8_t myDate;
uint8_t myMonth;
int myYear;

char temperature[8];
char humidity[8];
char pressure[8];
char altitude[8];

unsigned long previousSecond = 0;
const long interval = 1000;

int notes[] = 
{0,
/* C,  C#,   D,  D#,   E,   F,  F#,   G,  G#,   A,  A#,   B */
3817,3597,3401,3205,3030,2857,2703,2551,2404,2273,2146,2024,   // 3 (1-12)
1908,1805,1701,1608,1515,1433,1351,1276,1205,1136,1073,1012,   // 4 (13-24)
 956, 903, 852, 804, 759, 716, 676, 638, 602, 568, 536, 506,   // 5 (25-37)
 478, 451, 426, 402, 379, 358, 338, 319, 301, 284, 268, 253,   // 6 (38-50)
 239, 226, 213, 201, 190, 179, 169, 159, 151, 142, 134, 127 }; // 7 (51-62)

#define NOTE_G3  2551
#define NOTE_G4  1276
#define NOTE_C5  956
#define NOTE_E5  759
#define NOTE_G5  638
#define RELEASE  20
#define BPM      100

// notes in the melody:
int melody[] = {NOTE_E5,NOTE_E5,0,NOTE_E5,0,NOTE_C5,NOTE_E5,0,NOTE_G5,0,0,NOTE_G4};

// note durations: 4 = quarter note, 2 = half note, etc.:
int noteDurations[] = {4,4,4,4,4,4,4,4,4,2,4,4};

uint8_t myTime[6];
int serialindex = 0;

void setup() 
{
  Serial.begin(9600);
  delay(100);
  initBLE();
  pinMode(GREEN_LED,OUTPUT);//external LED active high
  pinMode(RED_LED,OUTPUT);//external LED active high
  pinMode(LED_BLUE,OUTPUT);//on board lED active low
  pinMode(speakerPin,OUTPUT);

  for (int thisNote = 0; thisNote < 12; thisNote++) {

     // to calculate the note duration, take one second
     // divided by the note type.
     // e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
     int noteDuration = 60*1000/BPM/noteDurations[thisNote];
     tone(speakerPin, (melody[thisNote]!=0)?(500000/melody[thisNote]):0,noteDuration-RELEASE);

     // blocking delay needed because tone() does not block
     delay(noteDuration);
  }
  noTone(speakerPin);
  digitalWrite(speakerPin,LOW);
//  tone(speakerPin,1000,100);
//  delay(50);
//  tone(speakerPin,4000,200);

  //turn OFF all LEDs
  digitalWrite(GREEN_LED,LOW);
  digitalWrite(RED_LED,LOW);
  digitalWrite(LED_BLUE,HIGH);
  
  //while(!Serial);
  hourFormat12();
  setTime(19,27,36,17,1,2024);
  
  
  // default settings
  unsigned status;
  status = bme.begin();  
    // You can also pass in a Wire library object like &Wire2
    // status = bme.begin(0x76, &Wire2)
    if (!status) {
        Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
        Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
        Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
        Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
        Serial.print("        ID of 0x60 represents a BME 280.\n");
        Serial.print("        ID of 0x61 represents a BME 680.\n");
        while (1) delay(10);
    }
    Serial.println("-- Begin Test --");

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  // Clear the buffer
  display.clearDisplay();
  display.display();
}

void loop() 
{
  uint8_t ch;
    // Forward from BLEUART to HW Serial
  while ( bleuart.available() )
  {  
    ch = (uint8_t) bleuart.read();
    Serial.write(ch);
    //setTime(myHour,myMinute,mySecond,myDate,myMonth,myYear);
    if(serialindex < 6)
        {
            /*Place the character in the string buffer:*/
            myTime[serialindex] = ch - '0';
            /*Go to the index to place the next character in:*/
            serialindex++;
        }
        else 
        {
          myHour = ((myTime[0])*10) + myTime[1];
        myMinute = ((myTime[2])*10) + myTime[3];
        mySecond = ((myTime[4])*10) + myTime[5];
        setTime(myHour,myMinute,mySecond,1,1,2024);
        for(int i=0;i<6;i++)
        {
          Serial.print(myTime[i]);
        }
          serialindex = 0;
        }
  }
  
  canvas.fillScreen(0);
  canvas.setTextColor(SSD1306_WHITE, SSD1306_BLACK);       // Draw white text
  canvas.setRotation(1);
  canvas.drawLine(0, 10, 32, 10, SSD1306_WHITE);
  canvas.drawLine(0, 84, 32, 84, SSD1306_WHITE);
  
  time_t t = now();

  int tempC = bme.readTemperature();//sht31.readTemperature();
  int humi = bme.readHumidity();//sht31.readHumidity();
  int pressure_hPa = bme.readPressure() / 100.0F;
  int altitude_m = bme.readAltitude(SEALEVELPRESSURE_HPA);


//  sprintf(temperature, "  %02dC", tempC);
//  sprintf(humidity, "  %02d%%", humi);
//  sprintf(pressure, "  %02dhPa", pressure_hPa);
//  sprintf(altitude, "  %02dhPa", altitude_m);
//
  sprintf(timeHour, "%02d", timeto12(hour(t)));
  sprintf(timeMinute, "%02d", minute(t));
  sprintf(timeSecond, "%02d", second(t));

  canvas.setFont();
  canvas.setTextSize(1);
  canvas.setCursor(0, 0);            // Start at top-left corner
  canvas.print("/////");
  //canvas.print(myTime);
  canvas.setCursor(0, 15); 
  if (isPM()) canvas.print(F("   PM"));
  else canvas.print(F("   AM"));

  
  //display.setTextSize(2);
  canvas.setFont(&DSEG7_Classic_Mini_Regular_15);
  canvas.setCursor(5, 40);
  canvas.print(timeHour);

  canvas.setCursor(5, 60);
  canvas.print(timeMinute);

  canvas.setCursor(5, 80);
  canvas.print(timeSecond);

  //unsigned long currentSecond = second(t);

  //if (currentSecond - previousSecond >= 2) 
  {
  //  previousSecond = currentSecond;
    
//    Serial.println(tempC);
//    Serial.println(humi);
//    Serial.println(pressure_hPa);
//    Serial.println(altitude_m);
    
    canvas.setFont();
    canvas.setTextSize(1);

    if (! isnan(tempC)) 
    {  // check if 'is not a number'
      Serial.print("Temp *C = "); 
      Serial.print(tempC); 
      Serial.print("\t\t");
      canvas.setCursor(0, 90);
      canvas.print("  ");canvas.print(tempC);canvas.print("C");
    }
    else Serial.println("Failed to read temperature");

    if (! isnan(humi)) 
    {  // check if 'is not a number'
      Serial.print("Hum. % = "); 
      Serial.println(humi);
      canvas.setCursor(0, 105);
      canvas.print("  ");canvas.print(humi);canvas.print("%");
      canvas.setCursor(0, 120);
      canvas.print("");canvas.print(pressure_hPa);canvas.print("hPa");
    } 
    else Serial.println("Failed to read humidity");
  }

  display.drawBitmap(0,0,canvas.getBuffer(),128,32,1,0);
  display.display();
  //display.startscrolldiagright(0x0F, 0x0F);
  
  for(int i = 0;i<5;i++)
  {
    digitalWrite(GREEN_LED,HIGH);
    delay(10);
    digitalWrite(GREEN_LED,LOW);
    delay(50);
  }
    for(int i = 0;i<5;i++)
  {
    digitalWrite(RED_LED,HIGH);
    delay(10);
    digitalWrite(RED_LED,LOW);
    delay(50);
  }
  delay(400);
}

int timeto12(int hourin24)
{
  if(hourin24 ==0) return 12;
  if(hourin24>12) return hourin24-12;
  else return hourin24;
}

void initBLE(void)
{
  Serial.println("Bluefruit52 BLEUART Example");
  Serial.println("---------------------------\n");

  // Setup the BLE LED to be enabled on CONNECT
  // Note: This is actually the default behavior, but provided
  // here in case you want to control this LED manually via PIN 19
  Bluefruit.autoConnLed(true);

  // Config the peripheral connection with maximum bandwidth 
  // more SRAM required by SoftDevice
  // Note: All config***() function must be called before begin()
  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);

  Bluefruit.begin();
  Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values
  //Bluefruit.setName(getMcuUniqueID()); // useful testing with multiple central connections
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  // To be consistent OTA DFU should be added first if it exists
  bledfu.begin();

  // Configure and Start Device Information Service
  bledis.setManufacturer("Adafruit Industries");
  bledis.setModel("Bluefruit Feather52");
  bledis.begin();

  // Configure and Start BLE Uart Service
  bleuart.begin();

  // Start BLE Battery Service
  blebas.begin();
  blebas.write(100);

  // Set up and start advertising
  startAdv();

  Serial.println("Please use Adafruit's Bluefruit LE app to connect in UART mode");
  Serial.println("Once connected, enter character(s) that you wish to send");
  
}

void startAdv(void)
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  // Include bleuart 128-bit uuid
  Bluefruit.Advertising.addService(bleuart);

  // Secondary Scan Response packet (optional)
  // Since there is no room for 'Name' in Advertising packet
  Bluefruit.ScanResponse.addName();
  
  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   * 
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html   
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
}

// callback invoked when central connects
void connect_callback(uint16_t conn_handle)
{
  // Get the reference to current connection
  BLEConnection* connection = Bluefruit.Connection(conn_handle);

  char central_name[32] = { 0 };
  connection->getPeerName(central_name, sizeof(central_name));

  Serial.print("Connected to ");
  Serial.println(central_name);
}

/**
 * Callback invoked when a connection is dropped
 * @param conn_handle connection where this event happens
 * @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
 */
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  (void) conn_handle;
  (void) reason;

  Serial.println();
  Serial.print("Disconnected, reason = 0x"); Serial.println(reason, HEX);
}
