#include "Arduino.h"
// Demo of MAX7219_Dot_Matrix library - sideways scrolling
// Author: Nick Gammon
// Date: 2 October 2015
//
// Updated by Ralph S Bacon to include some extra functions (speed/brightness/switch off)
// Date: April 2017
//
// Scrolls a pixel at a time.

// Libraries required
#include <SPI.h>
#include <bitBangedSPI.h>
#include "DHT.h"
#define DHTPIN A0     
#define DHTTYPE DHT11   

DHT dht(DHTPIN, DHTTYPE);

// Nick Gammon's library AMENDED BY ME if you want the 'switch off' extra function and to work in Eclipse
#include <MAX7219_Dot_Matrix.h>

// UPDATE THIS TO HOW MANY MAX7219 CHIPS ARE PRESENT
// THERE ARE FOUR CHIPS PER 4-IN-1 LED UNIT.
const byte chips = 4; // Two units
//
 // John Main added dewpoint code from : http://playground.arduino.cc/main/DHT11Lib
// Also added DegC output for Heat Index.
// dewPoint function NOAA
// reference (1) : http://wahiduddin.net/calc/density_algorithms.htm
// reference (2) : http://www.colorado.edu/geography/weather_station/Geog_site/about.htm
//
double dewPoint(double celsius, double humidity)
{
  // (1) Saturation Vapor Pressure = ESGG(T)
  double RATIO = 373.15 / (273.15 + celsius);
  double RHS = -7.90298 * (RATIO - 1);
  RHS += 5.02808 * log10(RATIO);
  RHS += -1.3816e-7 * (pow(10, (11.344 * (1 - 1 / RATIO ))) - 1) ;
  RHS += 8.1328e-3 * (pow(10, (-3.49149 * (RATIO - 1))) - 1) ;
  RHS += log10(1013.246);

  // factor -3 is to adjust units - Vapor Pressure SVP * humidity
  double VP = pow(10, RHS - 3) * humidity;

  // (2) DEWPOINT = F(Vapor Pressure)
  double T = log(VP / 0.61078); // temp var
  return (241.88 * T) / (17.558 - T);
}

// PIR related values
// PIR output connected to pin ? (can be any digital/analog IO pin)
#define PIR 2

// How long to keep the display running when no movement?
#define displayMinOnTime 10000

// To what pin is the PIR movement-detected LED connected
#define PIR_LED 8

// 12 chips (display modules), hardware SPI with load on D10
MAX7219_Dot_Matrix display(chips, 10);  // Chips / LOAD

unsigned long lastMoved = 0;
unsigned long MOVE_INTERVAL = 30;  // mS
int messageOffset;

// In this example message you can use the tilde character ~ to show the degree character like this:
//unsigned char message[] = "New York 88~F London 75~F Benny is the best cat ever!";

// Or follow the example below to create your string with special characters
unsigned char message[100] = {};

// -----------------------------------------------------------------------------------------
// SETUP	SETUP     SETUP     SETUP     SETUP     SETUP     SETUP     SETUP     SETUP
// -----------------------------------------------------------------------------------------
void setup() {

	display.begin();
	display.setIntensity(6); // initial brightness is moderate (original value 6)
	Serial.begin(9600);

  Serial.println("DHTxx test!");

 dht.begin();
 float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);

  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  float hif = dht.computeHeatIndex(f, h);
  float hic = dht.computeHeatIndex(t, h, false);
//
//serial monitor
//
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print("Heat index: ");
  Serial.print(hic);
  Serial.println(" *C ");
  Serial.print("Dew pt: ");
  Serial.print(dewPoint(t, h));
  Serial.println(" *C ");

	// Show what the PIR is doing (on=PIR detected movement)
	pinMode(PIR, INPUT_PULLUP);
	pinMode(PIR_LED, OUTPUT);
	digitalWrite(PIR_LED, LOW);

	// -----------------------------------------------------------------------------------------------
	// How to use the extended character set (see http://www.gammon.com.au/forum/?id=11516 "Font" area)
	// -----------------------------------------------------------------------------------------------

	// Create your string (in whichever function you require, we'll do it here in the setup as a demo)
	String msg = "Temperature ";
  msg += String(t);

	// This character is the degree symbol (small circle in top half of character space)
	msg += (char) 0xF8;

	// Now we just add more to the message as you want
	msg += "C   Humidity ";
  msg += String(h);
  msg += (char) 37;
	//msg += (char) 0xAb; // 1/2 symbol

	// Continue the message
	msg += "   Heat Index ";
  msg += String(hic);
  msg += (char) 0xF8;
  msg += "C   ";
  msg +="Dew pt ";
  msg += String(dewPoint(t, h));
  msg += (char) 0xF8;
  msg += "C ";
  
	//msg += (char) 0x01; // happy face symbol

	// Convert the entire string to a character array for sending to LED
	msg.toCharArray((char*)message, 100);

	// Debug message (might not display 100% correctly!)
	Serial.println((char *)message);

}  // end of setup

// -----------------------------------------------------------------------------------------
// UPDATE DISPLAY WITH NEW MESSAGE
// -----------------------------------------------------------------------------------------
void updateDisplay() {

	display.sendSmooth(message, messageOffset);

	// next time show one pixel onwards
	if (messageOffset++ >= (int) (strlen((char *)message) * 8)) messageOffset = -chips * 8;
}  // end of updateDisplay

// -----------------------------------------------------------------------------------------
// LOOP       LOOP     LOOP     LOOP     LOOP     LOOP     LOOP     LOOP     LOOP     LOOP
// -----------------------------------------------------------------------------------------
void loop() {

	static unsigned long int displayOnTime = millis();

	// update display if time is up
	if (millis() - lastMoved >= MOVE_INTERVAL) {
		updateDisplay();
		lastMoved = millis();
	}

	// New function: Set the delay between pixel moves
	getDelay(&MOVE_INTERVAL);

	// New function: Set the brightness (0=min, 15=max)
	display.setIntensity(getBrightness());

	// New function: Set the PIR led to show if movement detected
	if (digitalRead(PIR)) {
		display.shutdown(false);
		displayOnTime = millis();
		digitalWrite(PIR_LED, HIGH);
	}
	else {
		digitalWrite(PIR_LED, LOW);
		if (millis() > displayOnTime + displayMinOnTime) {
			display.shutdown(true);
		}
	}

}  // end of loop
//------------------------------------------------------------------------------------------
// Read New Values
//------------------------------------------------------------------------------------------
float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);


  float hif = dht.computeHeatIndex(f, h);
  float hic = dht.computeHeatIndex(t, h, false);

//  ------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------
// ROUTINES TO GET THE BRIGHTNESS AND SPEED FROM ANALOG PORTS A0 & A1 (10K POT)
// Wire up the potentiometers thus: +ve to one end, -ve to other end, analog pin to centre
// Note that the brightness control requires the updated library from Ralph S Bacon to work
// -----------------------------------------------------------------------------------------
void getDelay(unsigned long int *delayVar) {
	int delay = analogRead(14);
	*delayVar = map(delay, 0, 1023, 5, 75);
}

unsigned char getBrightness() {
	int bright = analogRead(15);
	return map(bright, 0, 1023, 0, 15);
}
