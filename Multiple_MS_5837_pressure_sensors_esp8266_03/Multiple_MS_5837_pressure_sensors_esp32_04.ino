// MIT License

// Martin Malthe Borch mmbo squiggly biosustain.dtu.dk DTU Biosustain
// Nicolas Padfield nicolas squiggly padfield.dk Fablab RUC
// Niels Jakob Larsen nilar squiggly dtu.dk DTU Biosustain

// v 0.4
// 2024-11-19

// Connecting multiple MS5837 pressure sensors to one esp32 by using wire (=twi = i2c) in software on multiple pins.
// Clock is the same pin for all sensors. Gnd and power must be connected all the time to all sensors. Data is a different pin per sensor (because they all have the same address and the address can not be changed, they can not all be on the same i2c bus)
// https://github.com/bluerobotics/BlueRobotics_MS5837_Library - looks better maintained and is in the arduino libraries repository
// https://github.com/TEConnectivity/MS5837_Arduino_Library - looks less well maintained
// https://docs.rs-online.com/c97e/A700000006772506.pdf
// board wemos d1 mini esp32
// board wemos d1 mini esp8266
// RS no 200-6130 Farnell no 3676988
// Based on Bluerobotics example, MIT license

// ESP32:
// SDA default is GPIO 21
// SCL default is GPIO 22

// ESP8266:
// SDA default is GPIO D1
// SCL default is GPIO D2
// Do not use (among others, probably) D0. D0 does not work.
// Use: D2 D2 D3 D4(onboardLED) D5 D6 D7 https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/

// pulled up with resistors. Typical values are 4.7k Ohm for 5V devices and 2.4k Ohm for 3.3V devices.
// in this setup: a 2.2k resistor up to 3.3v on clock. no pullup on data

// Sensor pinout from below. One is marked.

// VDD 2 brown   3 yellow SCL (22)
//
// GND 1* white  4 green SDA (21)

#include <Wire.h>
#include "MS5837.h"
#include <SD.h>
#include <SPI.h>

/*
PINOUT (on a Wemos d1 mini esp32)
Pins we know work for SPI/i2c (random post on internet says "Some of the ESP32 pins do not support SPI. You can only use pins 11,12,13,18,23,24,25,26,29 and 30 as SDA or SCK")

11 -
12 - 
13 TCK
18 IO18
23 IO23
24 -
25 IO25
26 IO26
29 -
30 - 

SD card tested good with:
IO5 CS
IO27 MOSI
IO33 CLK
IO19 MISO 

*/

const int chipSelect = 10;
File myFile;

#define numSensors 4

#define sensorsPowerPin 4


#define clkPin 13  // GPIO13 labelled TCK
#define s1pin 18
#define s2pin 25
#define s3pin 26
#define s4pin 23


//#define SDA 33
//#define SCL 32

#define useSD false

unsigned long lastSensorReset = 0;
unsigned int sensorResetInterval = 7200000;  // 2 hours

MS5837 sensor;
float temp = 7;
float pres = 7;
float tempa[numSensors];
float presa[numSensors];

bool headerWritten = false;  // Flag to track if header has been written

void setup() {

  Serial.begin(115200);
  Serial.println("Boot ");

  pinMode(sensorsPowerPin, OUTPUT);
  digitalWrite(sensorsPowerPin, HIGH);

  if (useSD) {
    File myFile = SD.open("test_2.txt", FILE_WRITE);

    if (!SD.begin(chipSelect)) {
      Serial.println("initialization failed");
      while (true)
        ;
    } else {
      Serial.println("initialization successful");

      if (myFile) {
        // Write header only if file is empty (file size is 0)
        if (myFile.size() == 0) {
          writeHeader();
        }
      }
    }
  }  // end if useSD
}

void loop() {

  if (millis() - lastSensorReset > sensorResetInterval) {
    lastSensorReset = millis();
    digitalWrite(sensorsPowerPin, LOW);
    delay(2000);
    digitalWrite(sensorsPowerPin, HIGH);
    delay(2000);
  }

  for (int i = 0; i < numSensors; i++) {
    sensRead(i);

    // Pull data from sensor function
    temp = sensor.temperature();
    pres = sensor.pressure();  // Pressure
    // Store this loop data
    tempa[i] = temp;
    presa[i] = pres;
  }
  delay(10);


  delay(1000);

  if (useSD) {
    if (myFile) {
      // Write pressure readings with 2 decimal places
      for (int i = 0; i < numSensors; i++) {
        myFile.print(presa[i], 2);
        if (i < numSensors - 1) myFile.print(",");
      }
      // Write temperature readings with 2 decimal places
      for (int i = 0; i < numSensors; i++) {
        myFile.print(",");
        myFile.print(tempa[i], 2);
        if (i < numSensors - 1) myFile.print(",");
      }
      myFile.println();  // New line after each set of readings
      myFile.flush();    // Ensure data is written to file
      Serial.println("Writing to file");
    } else {
      Serial.println("Error opening test.txt");
    }
  }
}

void sensRead(int num) {
  Serial.println("");
  Serial.print("Reading sensor no. ");
  Serial.println(num, DEC);
  Wire.end();

  switch (num) {
    case 0:
      Wire.begin(s1pin, clkPin);
      break;
    case 1:
      Wire.begin(s2pin, clkPin);
      break;
    case 2:
      Wire.begin(s3pin, clkPin);
      break;
    case 3:
      Wire.begin(s4pin, clkPin);
      break;
  }


  // Initialize pressure sensor

  bool initSuccess = false;

  for (int i = 0; i < 5; i++) {
    // try to init sensor
    if (sensor.init()) {
      initSuccess = true;
      Serial.print("Init success on attempt ");
      Serial.println(i);
      break;
    }
    delay(1);
  }

  delay(10);

  if (!initSuccess) {
    Serial.println("Init failed even after 5 attempts! Are power, SDA/SCL connected correctly?");
  } else {
    sensor.setModel(MS5837::MS5837_30BA);
    sensor.setFluidDensity(997);  // kg/m^3 (freshwater, 1029 for seawater)

    bool readSuccess = false;

    for (int i = 0; i < 5; i++) {
      // try to read sensor

      sensor.read();  // Update pressure and temperature readings

      if (sensor.pressure() < 30000 && sensor.pressure() > -30000 && sensor.temperature() > -200 && sensor.temperature() < 300) {
        readSuccess = true;  // success
        Serial.print("Read success on attempt ");
        Serial.println(i);
        break;
      }
      delay(1);
    }

    if (readSuccess) {

      // Print formatted sensor readings
      Serial.print("Pressure: ");
      Serial.print(sensor.pressure(), 2);  // 2 decimal places
      Serial.print(" mbar");
      Serial.print(" Temperature: ");
      Serial.print(sensor.temperature(), 2);  // 2 decimal places
      Serial.println(" deg C");
    } else {
      Serial.println("Read failed even after 5 attempts!");
    }
  }
}


void write() {

  delay(500);
  if (myFile) {
    // Write pressure readings with 2 decimal places
    for (int i = 0; i < numSensors; i++) {
      myFile.print(presa[i], 2);
      if (i < numSensors - 1) myFile.print(",");
    }

    // Write temperature readings with 2 decimal places
    for (int i = 0; i < numSensors; i++) {
      myFile.print(",");
      myFile.print(tempa[i], 2);
      if (i < numSensors - 1) myFile.print(",");
    }
    myFile.println();
    myFile.flush();

    Serial.println("Writing to file");
  } else {
    Serial.println("Error opening test.txt");
  }
}

void writeHeader() {
  if (myFile) {
    myFile.println("Pressure1(mbar),Pressure2(mbar),Pressure3(mbar),Temperature1(C),Temperature2(C),Temperature3(C)");
    myFile.flush();  // Ensure header is written
    Serial.println("CSV header written");
  }
}