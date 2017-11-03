#include <Adafruit_CircuitPlayground.h>
#include "adafruit_mini_codes.h"
#define IR_BITS 32

#if !defined(ARDUINO_SAMD_CIRCUITPLAYGROUND_EXPRESS)
  #error "Infrared support is only for the Circuit Playground Express, it doesn't work with the Classic version"
#endif

// Constants
int16_t Bright = 8;
int16_t Phase_duration = 300; // delay this long between phases
uint8_t N_phases = 10; // We have 10 different phases, 1 for each LED
int8_t Mono = 0;

uint8_t Pattern = 2;
int8_t Direction = 2;

// Variables
uint8_t current_phase = 0;
uint8_t flash_at_phase = 5;
uint8_t i,j;


void setup() {
  CircuitPlayground.begin();
  Serial.begin(9600);
  CircuitPlayground.irReceiver.enableIRIn(); // Start the receiver
}


void loop() {
  // Strategy: 
  // If we see another firefly flash when we're not, change our flash_at_phase randomly
  // If we don't see another firefly flash, or we're already in sync, don't change our flash_at_phase.

  if (current_phase == flash_at_phase) {
    // this is our phase to flash at. Let's go!
    flash();
    CircuitPlayground.irReceiver.enableIRIn(); //reset receiver
    delay(Phase_duration);
  }
  else{
    // Not our flash_at_phase. Show LED.
    show_phase_led();
    CircuitPlayground.irReceiver.enableIRIn(); //reset receiver
    delay(Phase_duration);

    // Did anyone else flash during this phase? If so, change flash_at_phase.
    if (read_ir()){
      flash_at_phase = (flash_at_phase + random(0, 5)) % N_phases; // Change flash_at randomly
    }
  }

  // increment phase
  current_phase = (current_phase + 1) % N_phases;
}


void flash() {
  // Send an IR signal
  CircuitPlayground.irSend.send(NEC, ADAF_MINI_1, IR_BITS); // what should IR_bits be set to??
  CircuitPlayground.irReceiver.enableIRIn(); //reset receiver

  // light all the LEDs
  CircuitPlayground.setBrightness(Bright);
  for(i=0;i<N_phases;i++) {
    //CircuitPlayground.setPixelColor(i, 255, 255,0);
  }

  delay(Phase_duration);
}


void show_phase_led(void) {
  //light LED for this phase
  CircuitPlayground.setBrightness(Bright);
  for(i=0;i<=N_phases;i++) {
    if(i==current_phase) {
      CircuitPlayground.setPixelColor(i, 255,0,0);
    } else  {
      CircuitPlayground.setPixelColor(i, 0,0,0);
    }
  }
}


bool read_ir(){
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
