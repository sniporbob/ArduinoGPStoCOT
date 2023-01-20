/*
 GPStoCOT

 This sketch receives GPS NMEA sentences over serial, then builds a
 Cursor-On-Target XML message. Some parameters are configurable below.
 The CoT is multicast out via the configured address and port.
 Uses rudimentary dynamic multicasting rate based on movement speed.

 This was designed for and tested with:
 Arduino Uno
 Arduino Ethernet Shield
 NEO-6M module (from Amazon, so probably counterfeit...)

 Other hardware is untested but likely to work,
 although slight changes may be required.

 Additional Arduino libraries required:
 TinyGPSPlus 1.0.3

 Created 2023 Jan 20
 by Sniporbob
 */


#include <Ethernet.h>
#include <EthernetUdp.h>
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>

////////////////////////////////
//// REQUIRED CONFIGURATION ////

char deviceName[] = "Tracker1"; // The name of the map marker
char type2525[] = "a-f-G-U-U-S-R"; // The 2525C marker icon
// https://spatialillusions.com/unitgenerator/ but use "a" in place of the initial "S",
// make sure the first two characters are lowercase, and drop the first "P".
// For example SFGPUC from the website should be input as a-f-G-U-C.

int staleMinuteCalc = 30; // The CoT stale time, in minutes. Unreasonable values untested.

// Enter a MAC address and IP address for your Ethernet Shield.
// There should be a sticker on the ethernet shield with the MAC (does not include the "0x").
// The IP address should be within the IP range of your network.
byte mac[] = {
  0x90, 0xA2, 0xB3, 0xC4, 0xD5, 0xE6
};
IPAddress lanIP(10, 13, 37, 222);

//// END REQUIRED CONFIG ////
/////////////////////////////
-------------------------------
////////////////////////////////
//// OPTIONAL CONFIGURATION ////

// This is the default ATAK multicast address/port for PLI. Recommend don't change.
IPAddress mcastIP(239, 2, 3, 1);
unsigned int mcastPort = 6969;

// Delays for dynamic sending of multicast packets (implemented based on speed).
// Units are milliseconds (1000 milliseconds = 1 second).
const int mcastRateSlow = 30000;
const int mcastRateMed = 20000;
const int mcastRateFast = 2000;

// Speed cutoffs for dynamic multicasting.
// Below medKPH uses mcastRateFast.
// Between medKPH and fastKPH uses mcastRateMed.
// Above fastKPH uses mcastRateFast.
const int medKPH = 7;
const int fastKPH = 20;

TinyGPSPlus gps; //create gps object
SoftwareSerial gpsSerial(2,3); // rx,tx pin assignments
const int gpsBaud = 9600;

//// END OPTIONAL CONFIG ////
/////////////////////////////

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

unsigned long mcastLastMillis = 0; // create timestamp variable for dynamic multicasting

void setup() {
  // You can use Ethernet.init(pin) to configure the CS pin
  Ethernet.init(10);  // Most Arduino shields
  //Ethernet.init(5);   // MKR ETH Shield
  //Ethernet.init(0);   // Teensy 2.0
  //Ethernet.init(20);  // Teensy++ 2.0
  //Ethernet.init(15);  // ESP8266 with Adafruit FeatherWing Ethernet
  //Ethernet.init(33);  // ESP32 with Adafruit FeatherWing Ethernet

  // Start serial connection. Don't try to serial.print() the CoT.
  // It's too big and will cause a boot loop.
  //Serial.begin(9600);

  // start the Ethernet
  Ethernet.begin(mac, lanIP);
  Udp.beginMulticast(mcastIP, mcastPort);

  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    //Serial.println("Ethernet cable is not connected.");
  }

  // Open GPS serial communication
  gpsSerial.begin(gpsBaud);

  //Serial.println("Setup Done");
}

void loop() {
  
  while(gpsSerial.available() > 0){ // check to see if gps data available via serial
    gps.encode(gpsSerial.read()); // if data available, send to TinyGPSPlus
  }
    // The below code performs simple dynamic updates based on speed.
    if (gps.location.isValid() && millis() % 100 == 0){ // Every 1/10th second, check if GPS location is valid
      int mcastRate = 0;

      // Check speed to see how often to multicast the CoT.
      if (gps.speed.isValid() && gps.speed.kmph() > fastKPH){
        mcastRate = mcastRateFast;
      }
      else if (gps.speed.isValid() && gps.speed.kmph() <= fastKPH && gps.speed.kmph() > medKPH){
        mcastRate = mcastRateMed;
      }
      else if (gps.speed.isValid()){
        mcastRate = mcastRateSlow;
      }

      if (millis() > mcastLastMillis + mcastRate){ // Don't do anything unless it's time to multicast a CoT.
        // Create latitude char and store double latitude into it
        char latchar[15];
        dtostrf(gps.location.lat(),3,6,latchar);

        // Same for longitude
        char lonchar[15];
        dtostrf(gps.location.lng(),4,6,lonchar);

        // Create speed char and store double speed into it
        char speedchar[10];
        dtostrf(gps.speed.kmph(),1,1,speedchar);

        // Create altitude char and store doule altitude into it
        char altitudechar[7];
        dtostrf(gps.altitude.meters(),1,1,altitudechar);

        //Create course char and store double course into it
        char coursechar[5];
        dtostrf(gps.course.deg(),1,1,coursechar);

        // Create error char, take hdop and multiple by gps precision, and store that into the error
        char errorchar[5];
        dtostrf(gps.hdop.hdop() * 4,1,1,errorchar);

        // Get the current date and time from the GPS
        char timeYear[16];
        itoa(gps.date.year(),timeYear,10);
        char timeMonth[8];
        itoa(gps.date.month(),timeMonth,10);
        char timeDay[8];
        itoa(gps.date.day(),timeDay,10);
        char timeHour[8];
        itoa(gps.time.hour(),timeHour,10);
        char timeMinute[16];
        itoa(gps.time.minute(),timeMinute,10);
        char timeSecond[8];
        itoa(gps.time.second(),timeSecond,10);
        char timeCentisecond[16];
        itoa(gps.time.centisecond(),timeCentisecond,10);

        // Create the variables to store the stale out date/time
        char staleMinute[16];
        char staleHour[8];
        char staleDay[8];
        char staleMonth[8];
        char staleYear[16];

        // This is a damned train wreck.
        // TODO: get rid of this crap and use proper date/time manipulation functions.
        // This mess calculates the stale out date/time by taking the current time and adding
        // the number of minutes from staleMinuteCalc. It does this poorly. You have been warned.
        if (gps.time.minute() + staleMinuteCalc >=60){
          int rollover = floor((gps.time.minute() + staleMinuteCalc) / 60);
          int tmpTime = (gps.time.minute() + staleMinuteCalc) % 60;
          itoa(tmpTime,staleMinute,10);

          if (gps.time.hour() + rollover >=24){
            tmpTime = (gps.time.hour() + rollover) % 24;
            rollover = floor((gps.time.hour() + rollover) / 24);
            itoa(tmpTime,staleHour,10);

            if((gps.date.month() == 1 || gps.date.month() == 3 || gps.date.month() == 5 || gps.date.month() == 7 || gps.date.month() == 8 || gps.date.month() == 10 || gps.date.month() == 12) && gps.date.day() + rollover > 31){
              tmpTime = (gps.date.day() + rollover) % 31;
              rollover = floor((gps.date.day() + rollover) / 31);
              itoa(tmpTime,staleDay,10);

              if (rollover >= 1){
                tmpTime = gps.date.month() + 1;
                rollover = floor((gps.date.month() + 1) / 12);
                itoa(tmpTime,staleMonth,10);
                if (rollover >= 1 ){
                  tmpTime = gps.date.year() + 1;
                  itoa(tmpTime,staleYear,10);                  
                }
                else {
                  //strcpy(staleYear,timeYear);
                  itoa(gps.date.year(),staleYear,10);
                }
              }
              else {
                strcpy(staleMonth,timeMonth);
              }
            }
            else if((gps.date.month() == 4 || gps.date.month() == 6 || gps.date.month() == 9 || gps.date.month() == 11) && gps.date.day() + rollover > 30){
              tmpTime = (gps.date.day() + rollover) % 30;
              rollover = (gps.date.day() + rollover) / 30;
              itoa(tmpTime,staleDay,10);

              if (rollover >= 1){
                tmpTime = gps.date.month() + 1;
                rollover = floor((gps.date.month() + 1) / 12);
                itoa(tmpTime,staleMonth,10);
                if (rollover >= 1 ){
                  tmpTime = gps.date.year() + 1;
                  itoa(tmpTime,staleYear,10);                  
                }
                else {
                  //strcpy(staleYear,timeYear);
                  itoa(gps.date.year(),staleYear,10);
                }
              }
              else {
                strcpy(staleMonth,timeMonth);
              }
            }
            else if (gps.date.month() == 2 && gps.date.day() + rollover > 28){
              tmpTime = (gps.date.day() + rollover) % 28;
              rollover = (gps.date.day() + rollover) / 28;
              itoa(tmpTime,staleDay,10);

              if (rollover >= 1){
                tmpTime = gps.date.month() + 1;
                rollover = floor((gps.date.month() + 1) / 12);
                itoa(tmpTime,staleMonth,10);
                if (rollover >= 1 ){
                  tmpTime = gps.date.year() + 1;
                  itoa(tmpTime,staleYear,10);                  
                }
                else {
                  //strcpy(staleYear,timeYear);
                  itoa(gps.date.year(),staleYear,10);
                }
              }
              else {
                strcpy(staleMonth,timeMonth);
              }
            }
            else {
              tmpTime = (gps.date.day() + rollover);
              itoa(tmpTime,staleDay,10);

              if (rollover >= 1){
                tmpTime = gps.date.month() + 1;
                rollover = floor((gps.date.month() + 1) / 12);
                itoa(tmpTime,staleMonth,10);
                if (rollover >= 1 ){
                  tmpTime = gps.date.year() + 1;
                  itoa(tmpTime,staleYear,10);                  
                }
                else {
                  //strcpy(staleYear,timeYear);
                  itoa(gps.date.year(),staleYear,10);
                }
              }
              else {
                strcpy(staleMonth,timeMonth);
                strcpy(staleYear,timeYear);
              }
            }
          }
          else {
            tmpTime = (gps.time.hour() + rollover);
            itoa(tmpTime,staleHour,10);
            itoa(gps.date.day(),staleDay,10);
            itoa(gps.date.month(),staleMonth,10);
            itoa(gps.date.year(),staleYear,10);
          }
        }
        else {
          int tmpTime = gps.time.minute() + staleMinuteCalc;
          itoa(tmpTime,staleMinute,10);
          itoa(gps.time.hour(),staleHour,10);
          itoa(gps.date.day(),staleDay,10);
          itoa(gps.date.month(),staleMonth,10);
          itoa(gps.date.year(),staleYear,10);
        }

        // Different components of CoT message. Data is inserted between these parts.
        char cotPart1[] = "<?xml version='1.0' encoding='UTF-8' standalone='yes'?><event version='2.0' uid='"; //string
        char cotPart2[] = "' type='";
        char cotPart3[] = "' time='";
        char cotPart4[] = "Z' start='";
        char cotPart5[] = "Z' stale='";
        char cotPart6[] = "Z' how='h-e'><point lat='";
        char cotPart7[] = "' lon='";
        char cotPart8[] = "' hae='";
        char cotPart9[] = "' ce='";
        char cotPart10[] = "' le='9999999'/><detail><track speed='";
        char cotPart11[] = "' course='";
        char cotPart12[] = "'/></detail></event>";

        // Create udp multicast CoT message packet and send it
        Udp.beginPacket(mcastIP, mcastPort);
        Udp.write(cotPart1);
        Udp.write(deviceName);
        Udp.write(cotPart2);
        Udp.write(type2525);

        Udp.write(cotPart3);
        Udp.write(timeYear);
        Udp.write("-");
        UdpWriteWithPrepend(timeMonth,2);
        Udp.write("-");
        UdpWriteWithPrepend(timeDay,2);
        Udp.write("T");
        UdpWriteWithPrepend(timeHour,2);
        Udp.write(":");
        UdpWriteWithPrepend(timeMinute,2);
        Udp.write(":");
        UdpWriteWithPrepend(timeSecond,2);
        Udp.write(".");
        UdpWriteWithPrepend(timeCentisecond,2);

        Udp.write(cotPart4);
        Udp.write(staleYear);
        Udp.write("-");
        UdpWriteWithPrepend(timeMonth,2);
        Udp.write("-");
        UdpWriteWithPrepend(timeDay,2);
        Udp.write("T");
        UdpWriteWithPrepend(timeHour,2);
        Udp.write(":");
        UdpWriteWithPrepend(timeMinute,2);
        Udp.write(":");
        UdpWriteWithPrepend(timeSecond,2);
        Udp.write(".");
        UdpWriteWithPrepend(timeCentisecond,2);

        Udp.write(cotPart5);
        Udp.write(timeYear);
        Udp.write("-");
        UdpWriteWithPrepend(staleMonth,2);
        Udp.write("-");
        UdpWriteWithPrepend(staleDay,2);
        Udp.write("T");
        UdpWriteWithPrepend(staleHour,2);
        Udp.write(":");
        UdpWriteWithPrepend(staleMinute,2);
        Udp.write(":");
        UdpWriteWithPrepend(timeSecond,2);
        Udp.write(".");
        UdpWriteWithPrepend(timeCentisecond,2);

        Udp.write(cotPart6);
        Udp.write(latchar);
        Udp.write(cotPart7);
        Udp.write(lonchar);
        Udp.write(cotPart8);
        Udp.write(altitudechar);
        Udp.write(cotPart9);
        Udp.write(errorchar);
        Udp.write(cotPart10);
        Udp.write(speedchar);
        Udp.write(cotPart11);
        Udp.write(coursechar);
        Udp.write(cotPart12);
        Udp.endPacket();

        // We just sent a CoT.
        // Set the time last sent equal to current millis.
        mcastLastMillis = millis();
      }

    }
}

void UdpWriteWithPrepend(char *input, int numDigits) {
  // Prepends zeros to a char array, making it numDigits long.
  // Will copy char array to temp buffer, set array to 0, then append buffer after the 0.
  // Afterwards, the array gets written to UDP.
  while (strlen(input) < numDigits) {
    char buf[sizeof(input)];
    strcpy(buf,input);
    strcpy(input,"0");
    strcat(input,buf);
  }
  Udp.write(input);
}