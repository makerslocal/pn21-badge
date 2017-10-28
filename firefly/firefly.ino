#include <Adafruit_CircuitPlayground.h>
#include "adafruit_mini_codes.h"
#define IR_BITS 32

#if !defined(ARDUINO_SAMD_CIRCUITPLAYGROUND_EXPRESS)
  #error "Infrared support is only for the Circuit Playground Express, it doesn't work with the Classic version"
#endif

#define JUST_ONE   2

uint8_t i,j,Phase, Pattern;
// uint8_t firefly = 0;
bool didweadjust = false;
int8_t Direction, Mono;
int16_t Bright, Speed;

void Show_Pattern(void) {
  CircuitPlayground.setBrightness(Bright);
  for(i=0;i<11;i++) {
    j= (20+i+Phase*Direction) % 10;
    switch (Pattern) {
      case JUST_ONE:
        if(i<2) {
          CircuitPlayground.setPixelColor(j, 255,0,0);
        } else  {
          CircuitPlayground.setPixelColor(j, 0,0,0);
        }
        break;
    }
  }
}

void go_firefly(void) {
  CircuitPlayground.setBrightness(Bright);
  for(i=0;i<11;i++) {
    CircuitPlayground.setPixelColor(i, 255, 255,0);
  }
      delay(Speed);
}

void setup() {
  CircuitPlayground.begin();
  Serial.begin(9600);
  CircuitPlayground.irReceiver.enableIRIn(); // Start the receiver
  Bright=8;  Phase=0;  Pattern=2;  Direction=1;  Speed=300;
  Mono=0;
}

void loop() {
  // to test swapping between send and recv inside the loop,
  // first we send the 'next' pattern via IR
  // if ((Phase + firefly) % 10 == 0) {
  if (Phase % 10 == 0) {
    // send the firefly signal!
    if (!didweadjust) {
    go_firefly();
    } else {
      didweadjust = false;
    }
    CircuitPlayground.irSend.send(NEC, ADAF_MINI_1, IR_BITS); // what should IR_bits be set to??
  }
    CircuitPlayground.irReceiver.enableIRIn(); // Start the receiver

  // First up, display whatever neopixel patterm we're doing
  Show_Pattern();

  // a small pause
  delay(Speed);

  // go to next phase next time
  Phase++;
  if (Phase >= 10) {
   Phase = 0;
  }

  // Did we get any infrared signals?
  if (! CircuitPlayground.irReceiver.getResults()) {
    return;
  }

  // Did we get any decodable messages?
  if (! CircuitPlayground.irDecoder.decode()) {
    CircuitPlayground.irReceiver.enableIRIn(); // Restart receiver
    return;
  }

  // We can print out the message if we want...
  CircuitPlayground.irDecoder.dumpResults(false);

  // Did we get any NEC remote messages?
  if (! CircuitPlayground.irDecoder.protocolNum == NEC) {
    CircuitPlayground.irReceiver.enableIRIn(); // Restart receiver
    return;
  }

  // What message did we get?
  switch(CircuitPlayground.irDecoder.value) {
  case ADAF_MINI_1:
    if (Phase < 8) {
      Phase = Phase - 1; // shorten phase
      didweadjust = true;
    }
    break;
  }
  //Restart receiver
  CircuitPlayground.irReceiver.enableIRIn();
}
