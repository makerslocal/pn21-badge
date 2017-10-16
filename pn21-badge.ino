#include <Adafruit_CircuitPlayground.h>
#include <Adafruit_NeoPixel.h>

#define NUM_LEDS 10

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_LEDS, 8, NEO_GRB + NEO_KHZ800);

void setup() {
	pinMode(13, OUTPUT);
	pixels.begin();
}

void loop() {
	digitalWrite(13, HIGH);
	pixels.setPixelColor(0, pixels.Color(255,0,0));
	pixels.show();
	delay(1000);
							
	digitalWrite(13, LOW);		
	pixels.setPixelColor(0, pixels.Color(0,0,0));
	pixels.show();
	delay(1000);							
}
