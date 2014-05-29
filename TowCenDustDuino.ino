/*
** TOW CENTER DUSTDUINO SKETCH V4.0 **
(C) 2014 by Matthew Schroyer
mentalmunition.com

    GNU General Public License
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

** DESCRIPTION **

This sketch is for a DustDuino multi-sensor, which measures
PM10 dust levels and noise. Noise is sampled at a rate of
20 Hertz (50 milliseconds), while a dust reading is taken
every 10 seconds. Peak and average readings are stored over
a one-minute period on SD card.

** CONNECTIONS **

SHARP GP2Y1010AU
Pin6 (Vcc/RED) -> 5V+
Pin5 (Vo/BLK) -> Arduino A1
Pin4 (S-GND/YEL) -> GND
Pin3 (LED/WHT) -> Arduino D5
Pin2 (LED-GND/GRN) -> Cap-
Pin1 (V-LED/BLU) -> Cap+

MIC
VCC -> 5V+
GND -> GND
OUT -> Arduino A0

REALTIME CLOCK
VCC -> 5V+
GND -> GND
SCL -> Arduino A5
SDA -> Arduino A4

SD CARD READ/WRITE
5V+ -> 5V+
GND -> GND
CS  -> Arduino D10
MOSI -> Arduino D11
SCK  -> Arduino D13
MISO -> Arduino D12

NOTIFICATION LED ON PIN 2

*/

#include <SD.h>
#include <Wire.h>
//#include <avr/wdt.h>
#include "RTClib.h"

RTC_DS1307 rtc;

// Chip Select on DustDuino for Tow is Digital 10
const int chipSelect = 10;
unsigned long starttime;
unsigned int sample;
      
      // Setting average, max, running total, and counters
      int AvgSound = 0;
      int MaxSound = 0;
      long SoundTotal = 0;
      long i = 0;
      
      double AvgDust = 0;
      double MaxDust = 0;
      double DustTotal = 0;
      long k = 0;
      
      int minutes = 0;
      

void setup() {
  // setting WTDT for 8 seconds
  //wdt_enable(WDTO_8S);
  
  Serial.begin(9600);
  pinMode(5,OUTPUT);
  
  // Dust reading notification LED on digital pin 2
 pinMode (2, OUTPUT);
  
  #ifdef AVR
  Wire.begin();
  #else
  Wire1.begin();
  #endif
  rtc.begin();
  
  Serial.print("Initializing SD card...");
  // CS Pin must be set to output
  pinMode(10, OUTPUT);
  
  // SD card check
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    return;
  }
  Serial.println("card initialized.");
  
  String startString = "Month,Day,Hour,Min,AvgDust,MaxDust,AvgSound,MaxSound";
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
    if (dataFile) {
    // Print first row with column headings in comma separated value format.  
    dataFile.println(startString);
    dataFile.close();
    Serial.println("Column headings printed...");
    Serial.println(startString);
    }
    
  starttime = millis();
  
  //wdt_reset();
}

void loop() {
    
    GetSound();
    //wdt_reset();
    
    // every 10 seconds, sample dust and increase counter;
    if ((millis() - starttime) > 10000) {
      GetDust();
      digitalWrite(2, HIGH);
      delay(100);
      digitalWrite(2, LOW);      
      minutes++;
      starttime = millis();
    }
    
    // when counter hits one minute, find time and log data
    if (minutes > 6) {
      LogData();
      minutes = 0;
      starttime = millis();
    }
    
}

void GetSound() {
  
  // Finds peak-to-peak amplitude of audio signal.
  // Samples audio at 20Hz, the lower limit of human hearing.
  // Upper limit of the microphone is 20kHz.
  // Posted on Adafrut: https://learn.adafruit.com/adafruit-microphone-amplifier-breakout?view=all
  
  unsigned long startMillis= millis(); // Start of sample window
  unsigned int peakToPeak = 0; // peak-to-peak level
 
  unsigned int signalMax = 0;
  unsigned int signalMin = 1024;
 
  // collect audio for 50 mS
  while (millis() - startMillis < 50)
  {
    sample = analogRead(0);
    if (sample < 1024) // toss out spurious readings
    {
    if (sample > signalMax)
    {
    signalMax = sample; // save just the max levels
    }
    else if (sample < signalMin)
    {
    signalMin = sample; // save just the min levels
    }
    }
  }
  peakToPeak = signalMax - signalMin; // max - min = peak-peak amplitude
  
  if (peakToPeak > MaxSound)
    {
      MaxSound = peakToPeak;
    }

    SoundTotal = SoundTotal + peakToPeak;
    i++;
}

void GetDust() {
    digitalWrite(5,LOW); // power on dust sensor LED
    delayMicroseconds(280);
    int dustSen = analogRead(A1); // read voltage off sensor pin5
    delayMicroseconds(40);
    digitalWrite(5,HIGH); // turn the LED off
    
    // Algorithm to find dust concentration from voltage, based on spec sheet
    double dustVal = ((((dustSen * 0.0049) * (0.172)) - 0.0999) * 100);
    if (dustVal < 0)
      {
        dustVal = 0;
      }

    if (dustVal > MaxDust) {
      MaxDust = dustVal;
    }
    
    DustTotal = dustVal + DustTotal;
    k++;
}

// Captures time and logs the data to SD
void LogData()
{
      DateTime now = rtc.now();

      AvgDust = DustTotal / k;
      AvgSound = SoundTotal / i;
      
      File dataFile = SD.open("datalog.txt", FILE_WRITE);
      if (dataFile) {
      // Print first row with column headings in comma separated value format.  
      dataFile.print(now.month(), DEC);
      dataFile.print(",");
      dataFile.print(now.day(), DEC);
      dataFile.print(",");
      dataFile.print(now.hour(), DEC);
      dataFile.print(",");
      dataFile.print(now.minute(), DEC);
      dataFile.print(",");
      dataFile.print(AvgDust);
      dataFile.print(",");
      dataFile.print(MaxDust);
      dataFile.print(",");
      dataFile.print(AvgSound);
      dataFile.print(",");
      dataFile.println(MaxSound);
      dataFile.close();
      
      // Also print to serial, if serial is connected
      Serial.print(now.month(), DEC);
      Serial.print(",");
      Serial.print(now.day(), DEC);
      Serial.print(",");
      Serial.print(now.hour(), DEC);
      Serial.print(",");
      Serial.print(now.minute(), DEC);
      Serial.print(",");
      Serial.print(AvgDust);
      Serial.print(",");
      Serial.print(MaxDust);
      Serial.print(",");
      Serial.print(AvgSound);
      Serial.print(",");
      Serial.println(MaxSound);
      
      // Reset mean, max, running totals, and counters
      AvgSound = 0;
      MaxSound = 0;
      SoundTotal = 0;
      i = 0;
      
      AvgDust = 0;
      MaxDust = 0;
      DustTotal = 0;
      k = 0;
      }
}

