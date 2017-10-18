#include <Adafruit_CircuitPlayground.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_SleepyDog.h>

#define NUM_LEDS 10
#define DEBUG true

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
	Serial.begin(115200);

	pinMode(13, OUTPUT);
	pixels.begin();
	pixels.setPixelColor(0, pixels.Color(0,0,1));
	pixels.show();
}

void loop() {
	//Serial
	if ( Serial.read() != -1 ) { 
		//someone sent us anything
		Serial.println("PhreakNIC 21 Badge by shapr and hfuller - https://github.com/makerslocal/pn21-badge");
		Serial.println("There are no commands available via serial (yet?). Patches welcome!");
		while ( Serial.read() != -1 ); //empty the receive buffer.
		Serial.flush(); //wait for all data to be sent.
	}

	//Pilot
	digitalWrite(13, HIGH);
	sleep(75);
	digitalWrite(13, LOW);
	sleep(1925);
							
}
