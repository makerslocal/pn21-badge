#include <Adafruit_CircuitPlayground.h>
#include <Adafruit_SleepyDog.h>
#include <FlashAsEEPROM.h>

#include "adafruit_mini_codes.h"
#include "FidgetSpinner.h"
#include "PeakDetector.h"

#define NUM_LEDS 10
#define IR_BITS 32
#define FIDGET_AXIS CircuitPlayground.motionX()
#define FIDGET_INVERT_AXIS true
#define DEBUG false

enum Mode { UPRIGHT, TABLE, FACE_DOWN, OTHER };
typedef struct {
	uint32_t primary;
	uint32_t secondary;
} colorCombo;
colorCombo fidgetColors[] = {
  { .primary = 0x960031, .secondary = 0x080808 },
  { .primary = 0xFF0000, .secondary = 0x000000 },  // Red to black
  { .primary = 0x00FF00, .secondary = 0x000000 },  // Green to black
  { .primary = 0x0000FF, .secondary = 0x000000 },  // Blue to black
  { .primary = 0xFF0000, .secondary = 0x00FF00 },  // Red to green
  { .primary = 0xFF0000, .secondary = 0x0000FF },  // Red to blue
  { .primary = 0x00FF00, .secondary = 0x0000FF }   // Green to blue
};
PeakDetector peakDetector(30, 20.0, 0.1);
FidgetSpinner fidgetSpinner(0.8);
int16_t fireflyBright = 8;
int16_t fireflyPhaseDuration = 300; // delay this long between phases
uint8_t fireflyPhases = 10; // We have 10 different phases, 1 for each LED
int fadeValues [] = { 255, 230, 200, 150, 100, 50, 10 };

// Variables
uint8_t fireflyCurrentPhase = 0;
int i,j;
uint8_t myId = 255;
Mode proposedMode = OTHER;
Mode detectedMode = OTHER;
Mode currentMode = OTHER;
sensors_event_t event;
uint32_t lastMS = millis();
uint32_t peakDebounce = 0;
uint32_t buttonDebounce = 0;
int currentAnimation = 3;
int currentColor = 0;

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
	for ( j=6; j>=0; j-- ) {
		sleep(20);
		setAllPixelsColor(fadeValues[j],fadeValues[j],0);
	}
	for ( j=0; j<=6; j++ ) {
		sleep(20);
		setAllPixelsColor(fadeValues[j],fadeValues[j],0);
	}
}
void setAllPixelsColor(uint8_t r, uint8_t g, uint8_t b) {
	for ( int x=0; x<NUM_LEDS; x++ ) {
		CircuitPlayground.setPixelColor(x, r,g,b);
	}
}
void fireflyShowPhaseLed(void) {
	if (DEBUG) {
		//light LED for this phase
		CircuitPlayground.setBrightness(fireflyBright);
		for(i=0;i<=fireflyPhases;i++) {
			if(i==fireflyCurrentPhase) {
				CircuitPlayground.setPixelColor(i, 255,0,0);
			} else	{
				CircuitPlayground.setPixelColor(i, 0,0,0);
			}
		}
	} else {
		//Don't do all this battery-using stuff if we're not in debug
		//CircuitPlayground.clearPixels(); //FIXME - if we fade out the blip we don't need this
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
		Serial.println("Got IR firefly signal");
		return true;
	}

	return false;
}
void fillPixels(const uint32_t color) {
  // Set all the pixels on CircuitPlayground to the specified color.
  for (int i=0; i<CircuitPlayground.strip.numPixels();++i) {
    CircuitPlayground.strip.setPixelColor(i, color);
  }
}

float constrainPosition(const float pos) {
  // Take a continuous positive or negative value and map it to its relative positon
  // within the range 0...<10 (so it's valid as an index to CircuitPlayground pixel
  // position).
  float result = fmod(pos, CircuitPlayground.strip.numPixels());
  if (result < 0.0) {
    result += CircuitPlayground.strip.numPixels();
  }
  return result;
}

float lerp(const float x, const float x0, const float x1, const float y0, const float y1) {
  // Linear interpolation of value y within y0...y1 given x and range x0...x1.
  return y0 + (x-x0)*((y1-y0)/(x1-x0));
}

uint32_t primaryColor() {
  return fidgetColors[currentColor].primary;
}
uint32_t secondaryColor() {
  return fidgetColors[currentColor].secondary;
}

uint32_t colorLerp(const float x, const float x0, const float x1, const uint32_t c0, const uint32_t c1) {
  // Perform linear interpolation of 24-bit RGB color values.
  // Will return a color within the range of c0...c1 proportional to the value x within x0...x1.
  uint8_t r0 = (c0 >> 16) & 0xFF;
  uint8_t g0 = (c0 >> 8) & 0xFF;
  uint8_t b0 = c0 & 0xFF;
  uint8_t r1 = (c1 >> 16) & 0xFF;
  uint8_t g1 = (c1 >> 8) & 0xFF;
  uint8_t b1 = c1 & 0xFF;
  uint32_t r = int(lerp(x, x0, x1, r0, r1));
  uint32_t g = int(lerp(x, x0, x1, g0, g1));
  uint32_t b = int(lerp(x, x0, x1, b0, b1));
  return (r << 16) | (g << 8) | b;
}

// Animation functions:
void animateDots(float pos, int count) {
  // Simple discrete dot animation.  Spins dots around the board based on the specified
  // spinner position.  Count specifies how many dots to display, each one equally spaced
  // around the pixels (in practice any count that 10 isn't evenly divisible by will look odd).
  // Count should be from 1 to 10 (inclusive)!
  fillPixels(secondaryColor());
  // Compute each dot's position and turn on the appropriate pixel.
  for (int i=0; i<count; ++i) {
    float dotPos = constrainPosition(pos + i*(float(CircuitPlayground.strip.numPixels())/float(count)));
    CircuitPlayground.strip.setPixelColor(int(dotPos), primaryColor());
  }
  CircuitPlayground.strip.show();
}

void animateSine(float pos, float frequency) {
  // Smooth sine wave animation.  Sweeps a sine wave of primary to secondary color around
  // the board pixels based on the specified spinner position.
  // Compute phase based on spinner position.  As the spinner position changes the phase will
  // move the sine wave around the pixels.
  float phase = 2.0*PI*(constrainPosition(pos)/10.0);
  for (int i=0; i<CircuitPlayground.strip.numPixels();++i) {
    // Use a sine wave to compute the value of each pixel based on its position for time
    // (and offset by the global phase that depends on fidget spinner position).
    float x = sin(2.0*PI*frequency*(i/10.0)+phase);
    CircuitPlayground.strip.setPixelColor(i, colorLerp(x, -1.0, 1.0, primaryColor(), secondaryColor()));
  }
  CircuitPlayground.strip.show();
}



void setup() {
	Serial.begin(115200);
	Serial.println("starting boot");

	myId = EEPROM.read(1);
	while ( myId == 255 or myId == 0 ) {
		myId = random(1, 254);
		EEPROM.write(1, myId);
	}
	Serial.print("My ID is "); Serial.println(myId);

	CircuitPlayground.begin();
	CircuitPlayground.setAccelRange(LIS3DH_RANGE_16_G);
	CircuitPlayground.irReceiver.enableIRIn();

	Serial.println("booted");
}

void loop() {
	//Serial
	if ( Serial.read() != -1 ) { 
		//someone sent us anything
		Serial.println("PhreakNIC 21 Badge by shapr and hfuller - https://github.com/makerslocal/pn21-badge");
		Serial.println("There are no commands available via serial (yet?). Patches welcome!");
		Serial.print("My unique ID is "); Serial.println(myId);
		while ( Serial.read() != -1 ); //empty the receive buffer.
		Serial.flush(); //wait for all data to be sent.
	}

	//Mode change
	CircuitPlayground.lis.getEvent(&event);
	if ( DEBUG ) {
		Serial.print("Orientation: "); Serial.print(event.acceleration.x); 
		Serial.print(','); Serial.print(event.acceleration.y); 
		Serial.print(','); Serial.println(event.acceleration.z);
	}
	if ( abs(event.acceleration.y) > 5 && abs(event.acceleration.x) < 3 && abs(event.acceleration.z) < 3 ) {
		detectedMode = UPRIGHT;
	} else if ( event.acceleration.z > 5 ) {
		detectedMode = TABLE;
	} else if ( event.acceleration.z < -5 ) {
		detectedMode = FACE_DOWN;
	} else {
		detectedMode = OTHER;
	}
	if ( detectedMode != currentMode && detectedMode == proposedMode ) {
		currentMode = detectedMode;
		Serial.println("Mode changed");
	} else if ( detectedMode != currentMode ) {
		proposedMode = detectedMode;
	}

	//Pilot
	if ( DEBUG or Serial ) {
		CircuitPlayground.redLED(true);
		sleep(5);
		CircuitPlayground.redLED(false);
	}

	if ( currentMode == UPRIGHT ) {
		//Firefly
		// Strategy: 
		// If we see another firefly flash when we're not, change our fireflyFlashAtPhase randomly
		// If we don't see another firefly flash, or we're already in sync, don't change our fireflyFlashAtPhase.

		if (!fireflyCurrentPhase) {
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
				fireflyCurrentPhase += fireflyCurrentPhase < fireflyPhases/2 ? -1 : 1;
			}
		}
		// increment phase
		fireflyCurrentPhase = (fireflyCurrentPhase + 1) % fireflyPhases;
	} else if ( currentMode == TABLE ) {
		//Fidget
		if ( CircuitPlayground.rightButton() ) {
			currentColor = (currentColor + 1) % (sizeof(fidgetColors)/sizeof(colorCombo));
		}
		uint32_t currentMS = millis();
		uint32_t deltaMS = currentMS - lastMS;  // Time in milliseconds.
		float deltaS = deltaMS / 1000.0;        // Time in seconds.
		lastMS = currentMS;

		// Grab the current accelerometer axis value and look for a sudden peak.
		float accel = FIDGET_AXIS;
		int result = peakDetector.detect(accel);

		if ((result != 0) && (currentMS >= peakDebounce)) {
			peakDebounce = currentMS + 500;
			// Invert accel because accelerometer axis positive/negative is flipped
			// with respect to pixel positive/negative movement.
			if (FIDGET_INVERT_AXIS) {
				fidgetSpinner.spin(-accel);
			} else {
				fidgetSpinner.spin(accel);
			}
		}
		// Update the spinner position and draw the current animation frame.
		float pos = fidgetSpinner.getPosition(deltaS);
		switch (currentAnimation) {
			case 0:
				// Single dot.
				animateDots(pos, 1);
				break;
			case 1:
				// Two opposite dots.
				animateDots(pos, 2);
				break;
			case 2:
				// Sine wave with one peak.
				animateSine(pos, 1.0);
				break;
			case 3:
				// Sine wave with two peaks.
				animateSine(pos, 2.0);
				break;
		}


	}
}

