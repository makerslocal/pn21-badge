#include <Adafruit_CircuitPlayground.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_SleepyDog.h>

#define NUM_LEDS 10

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_LEDS, 8, NEO_GRB + NEO_KHZ800);

void sleep(unsigned long ms) {
	if ( millis() < 10000 || Serial ) {
		//Either we haven't been running for long, or we are connected to a PC.
		delay(ms);
	} else {
		Watchdog.sleep(ms);
	}
}

void setup() {
	pinMode(13, OUTPUT);
	pixels.begin();
}

void loop() {
	digitalWrite(13, HIGH);
	sleep(75);
	digitalWrite(13, LOW);
	sleep(1925);
							
	digitalWrite(13, LOW);		
	pixels.setPixelColor(0, pixels.Color(0,0,0));
	pixels.show();
	delay(1000);							
}
