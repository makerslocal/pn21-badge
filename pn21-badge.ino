#include <Adafruit_CircuitPlayground.h>
#include <Adafruit_SleepyDog.h>

#include "adafruit_mini_codes.h"

#define NUM_LEDS 10
#define IR_BITS 32

#define DEBUG true

// Constants
int16_t fireflyBright = 8;
int16_t fireflyPhaseDuration = 300; // delay this long between phases
uint8_t fireflyPhases = 10; // We have 10 different phases, 1 for each LED

// Variables
uint8_t fireflyCurrentPhase = 0;
uint8_t fireflyFlashAtPhase = 5;
uint8_t i,j;


void sleep(unsigned long ms) {
	if ( millis() < 10000 || Serial ) {
		//Either we haven't been running for long, or we are connected to a PC.
		delay(ms);
	} else {
		Watchdog.sleep(ms);
	}
}
void fireflyFlash() {
	// Send an IR signal
	CircuitPlayground.irSend.send(NEC, ADAF_MINI_1, IR_BITS); // what should IR_bits be set to??
	CircuitPlayground.irReceiver.enableIRIn(); //reset receiver

	// light all the LEDs
	CircuitPlayground.setBrightness(fireflyBright);
	for(i=0;i<fireflyPhases;i++) {
		CircuitPlayground.setPixelColor(i, 255, 255,0);
	}

	sleep(fireflyPhaseDuration);
}
void fireflyShowPhaseLed(void) {
	//light LED for this phase
	CircuitPlayground.setBrightness(fireflyBright);
	for(i=0;i<=fireflyPhases;i++) {
		if(i==fireflyCurrentPhase) {
			CircuitPlayground.setPixelColor(i, 255,0,0);
		} else	{
			CircuitPlayground.setPixelColor(i, 0,0,0);
		}
	}
}
bool readIr(){
	// Not sure of the logic here, CHECK PLZ
	// Did we get at least one IR signal?
	if (! CircuitPlayground.irReceiver.getResults()) {
		return false;
	}

	// Did we get any decodable messages?
	if (! CircuitPlayground.irDecoder.decode()) {
		return false;
	}

	// We can print out the message if we want...
	CircuitPlayground.irDecoder.dumpResults(false);

	// What message did we get?
	switch(CircuitPlayground.irDecoder.value) {
	case ADAF_MINI_1:
		return true;
	}

	return false;
}


void setup() {
	Serial.begin(115200);
	CircuitPlayground.begin();
	CircuitPlayground.irReceiver.enableIRIn();

	pinMode(13, OUTPUT);
	CircuitPlayground.setPixelColor(0, 0,0,1);
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

	//Firefly
	// Strategy: 
	// If we see another firefly flash when we're not, change our fireflyFlashAtPhase randomly
	// If we don't see another firefly flash, or we're already in sync, don't change our fireflyFlashAtPhase.

	if (fireflyCurrentPhase == fireflyFlashAtPhase) {
		// this is our phase to flash at. Let's go!
		fireflyFlash();
		CircuitPlayground.irReceiver.enableIRIn(); //reset receiver
		sleep(fireflyPhaseDuration);
	}
	else{
		// Not our fireflyFlashAtPhase. Show LED.
		fireflyShowPhaseLed();
		CircuitPlayground.irReceiver.enableIRIn(); //reset receiver
		sleep(fireflyPhaseDuration);

		// Did anyone else flash during this phase? If so, change fireflyFlashAtPhase.
		if (readIr()){
			fireflyFlashAtPhase = (fireflyFlashAtPhase + random(0, 5)) % fireflyPhases; // Change flash_at randomly
		}
	}
	// increment phase
	fireflyCurrentPhase = (fireflyCurrentPhase + 1) % fireflyPhases;
}

