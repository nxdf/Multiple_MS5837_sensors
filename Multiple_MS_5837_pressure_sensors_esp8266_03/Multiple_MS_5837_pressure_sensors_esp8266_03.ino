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

#define NUMSENSORS 2
#define CLK D2

MS5837 sensor;

void setup() {

  Serial.begin(115200);

  Serial.println("Boot ");
}

void loop() {

  for (int i = 0; i < NUMSENSORS; i++) {
    sensRead(i);
  }
  delay(2000);
}

void sensRead(int num) {

  Serial.println("");
  Serial.print("Reading sensor no. ");
  Serial.println(num, DEC);

  switch (num) {
    case 0:
      Wire.begin(D1, CLK);  // sensor 0, which is data (SDA) D1 and clock (SCL) D2
      break;
    case 1:
      Wire.begin(D6, CLK);  // sensor 1, which is data (SDA) D0 and (SCL) D2
      break;
  }

  sensInit();

  sensor.read();  // Update pressure and temperature readings

  Serial.print("Pressure: ");
  Serial.print(sensor.pressure());
  Serial.print(" mbar");

  Serial.print(" Temperature: ");
  Serial.print(sensor.temperature());
  Serial.println(" deg C");

  /*
  Serial.print(" Depth: ");
  Serial.print(sensor.depth());
  Serial.print(" m");

  Serial.print(" Altitude: ");
  Serial.print(sensor.altitude());
  Serial.println(" m above mean sea level");
*/
}

void sensInit() {
  // Initialize pressure sensor. Returns true if initialization was successful. We can't continue with the rest of the program unless we can initialize the sensor

  if (!sensor.init()) {
    Serial.println("Init failed! Are power, SDA/SCL connected correctly?");
  }

  sensor.setModel(MS5837::MS5837_30BA);
  sensor.setFluidDensity(997);  // kg/m^3 (freshwater, 1029 for seawater)
}
