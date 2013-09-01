// HIH_6130_1  - Arduino for Nimbits
// 
// Arduino                HIH-6130
// SCL (Analog 5) ------- SCL (term 3)
// SDA (Analog 4) ------- SDA (term 4)
//
// Note 2.2K pullups to 5 VDC on both SDA and SCL
//
// Pin4 ----------------- Vdd (term 8) 
//
// Illustrates how to measure relative humidity and temperature.
//
// copyright, Peter H Anderson, Baltimore, MD, Nov, '11
// You may use it, but please give credit.  

/** ----- Revisions
2013Aug31  bboyes switching to support Nimbits vs simple GAE app
2013Aug31  bboyes Added delays in startup and between attempts to connect
           Check return value from client.connect; report error to terminal
           
2013 Aug 27 bboyes adding Ethernet support for GAE demo
----- **/

    
#include <Wire.h> //I2C library

#include <SPI.h>      // Ethernet also needs SPI for W5100
#include <Ethernet.h>
#include <util.h>  // small Ethernet sub-library

// Stuff for Nimbits
#include "Nimbits.h"  // not in Arduino libraries?
//#include <PString.h>  // not needed
#include <stdlib.h>
#define MAX_KEY_SIZE 64  // What are these??
#define CATEGORY 2
#define POINT 1
#define SUBSCRIPTION 5
//nimbits settings, set the instance name (nimbits-02 is the public cloud on https://cloud.nimbits.com) the email of the account owner, and a read write key they have created.
String instance = "smartstatmicrodata";
char owner[] = "semesa2@gmail.com";
String readWriteKey = "3cad4cad-5cad-6cad-7cad-8cad-9cad10c";
//buffers for holding the ids of our entities provided by the server.
char pointId[MAX_KEY_SIZE];
char folderId[MAX_KEY_SIZE]; 
//the names of the entities that will either be created or queried on startup
char folder[] = "RD";
char point[] = "RD Alerting Point";
// Nimbits instance I guess
Nimbits nimbits(instance, owner, readWriteKey);

byte fetch_humidity_temperature(unsigned int *p_Humidity, unsigned int *p_Temperature);
void print_float(float f, int num_digits);

#define TRUE 1
#define FALSE 0

uint32_t dtime;  // delay time in msec

#define MIN_15 900000  // msec in 15 min = 900,000
#define MIN_1 60000 // msec in 1 minute = 60,000
#define MIN_5 300000 // msec in 5 min

// assign a MAC address for the ethernet controller.
// fill in your address here:
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
// assign an IP address for the controller:
IPAddress ip(192,168,1,20);
IPAddress gateway(192,168,1,1);	
IPAddress subnet(255, 255, 255, 0);


//char server[] =  "appengine.google.com";
char server[] =  "smartstatmicrodata.appspot.com";

EthernetClient client;

void setup(void)
{
   Serial.begin(115200);
   Wire.begin();
   pinMode(4, OUTPUT);
   digitalWrite(4, HIGH); // this turns on the HIH3610
   Serial.println("HIH6130 Nimbits 2013 Aug 31"); 
   delay(3000);  // wait for all devices to stabilize and complete any internal startup
   Serial.print("Sample interval ");  // just to be sure things are working
   dtime = MIN_1;  //delay between readings
   Serial.print(dtime/1000);
   Serial.println(" sec");
   
  Serial.print("Ethernet setup... ");
  // start the Ethernet connection, there is no return to check
  Ethernet.begin(mac, ip);
  delay (1000);    // is this needed? See http://arduino.cc/en/Reference/EthernetClient
  
  if (client.connect(server, 80)) 
  {
    Serial.print ("Server ");
    Serial.print (server);
    Serial.println(" connected!");
    client.stop();
  } 
  else 
    {
      Serial.println("connection FAILED!");
    }
   
delay (1000);  // wait before trying to reconnect
  
   Serial.println("Time Stat  RH%   TempC");
   
}
    
void loop(void)
{
   byte _status;
   unsigned int H_dat, T_dat;
   float RH, T_C;
   
   
   while(1)
   {
      _status = fetch_humidity_temperature(&H_dat, &T_dat);
      Serial.print("@");
      Serial.print(millis()/1000);
      
      switch(_status)
      {
          case 0:  Serial.print(" Norm ");
                   break;
          case 1:  Serial.print(" Stale Data! ");
                   break;
          case 2:  Serial.print(" In command mode! ");
                   break;
          default: Serial.print(" Diagnostic! "); 
                   break; 
      }       
    
      RH = (float) H_dat * 6.10e-3;
      T_C = (float) T_dat * 1.007e-2 - 40.0;
      
    print_float(RH, 2);
    Serial.print(" % ");
    print_float(T_C, 2);
    Serial.println(" C");
      
        //Send sensor values to cloud
  String stemp = String(T_C,DEC);
  String shum = String(RH,DEC);
  
  nimbits.recordValue(T_C,"degC","semesa2@gmail.com/temperature");
    
    Serial.print ("Sent to Nimbits: RH=");
    Serial.print (shum);
    Serial.print (" TempC=");
    Serial.println (stemp);
        
  Serial.print ("Wait ");
  Serial.print (dtime/1000);
  Serial.println(" sec");
   delay(dtime);
   }
}



byte fetch_humidity_temperature(unsigned int *p_H_dat, unsigned int *p_T_dat)
{
      byte address, Hum_H, Hum_L, Temp_H, Temp_L, _status;
      unsigned int H_dat, T_dat;
      address = 0x27;;
      Wire.beginTransmission(address); 
      Wire.endTransmission();
      delay(100);
      
      Wire.requestFrom((int)address, (int) 4);
      Hum_H = Wire.receive();
      Hum_L = Wire.receive();
      Temp_H = Wire.receive();
      Temp_L = Wire.receive();
      Wire.endTransmission();
      
      _status = (Hum_H >> 6) & 0x03;
      Hum_H = Hum_H & 0x3f;
      H_dat = (((unsigned int)Hum_H) << 8) | Hum_L;
      T_dat = (((unsigned int)Temp_H) << 8) | Temp_L;
      T_dat = T_dat / 4;
      *p_H_dat = H_dat;
      *p_T_dat = T_dat;
      return(_status);
}
   
void print_float(float f, int num_digits)
{
    int f_int;
    int pows_of_ten[4] = {1, 10, 100, 1000};
    int multiplier, whole, fract, d, n;

    multiplier = pows_of_ten[num_digits];
    if (f < 0.0)
    {
        f = -f;
        Serial.print("-");
    }
    whole = (int) f;
    fract = (int) (multiplier * (f - (float)whole));

    Serial.print(whole);
    Serial.print(".");

    for (n=num_digits-1; n>=0; n--) // print each digit with no leading zero suppression
    {
         d = fract / pows_of_ten[n];
         Serial.print(d);
         fract = fract % pows_of_ten[n];
    }
}      

