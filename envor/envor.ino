//
//    FILE: MCP4921_test.ino
//  AUTHOR: Rob Tillaart
// PURPOSE: test MCP4921 lib
//    DATE: 2021-02-03
//     URL: https://github.com/RobTillaart/MCP4921


#include "MCP_DAC.h"

#define PPQN 24
unsigned long bpm_clock() {
  return millis()/PPQN;
}
#include "Envelopes.h"


#define IN_GATE     A0
#define IN_ATTACK   A3
#define IN_DECAY    A4
#define IN_SUSTAIN  A2
#define IN_RELEASE  A1
//#define IN_DECAY    A3
//#define IN_RELEASE  A2

// MCP4921 MCP(11, 13);  // SW SPI
MCP4922 MCP;  // HW SPI

volatile int x;
uint32_t start, stop;

void setup()
{
  Serial.begin(115200);
  Serial.println(__FILE__);

  MCP.begin(10);
  
  Serial.print("MCP_DAC_LIB_VERSION: ");
  Serial.println(MCP_DAC_LIB_VERSION);
  Serial.println();
  Serial.print("CHANNELS:\t");
  Serial.println(MCP.channels());
  Serial.print("MAXVALUE:\t");
  Serial.println(MCP.maxValue());
  delay(100);

  pinMode(IN_GATE, INPUT);
  pinMode(IN_ATTACK, INPUT);
  pinMode(IN_RELEASE, INPUT);

  //performance_test();
  //analogWrite_test();

  initialise_envelopes();

  Serial.println("\nDone...");
}


void analogWrite_test()
{
  Serial.println();
  Serial.println(__FUNCTION__);
  for (int channel = 0; channel < MCP.channels(); channel++)
  {
    Serial.println(channel);
    for (uint16_t value = 0; value < MCP.maxValue(); value += 0xFF)
    {
      MCP.analogWrite(value, channel);
      Serial.print(value);
      Serial.print("\t");
      Serial.println(analogRead(IN_GATE));
      delay(10);
    }
  }
}


void performance_test()
{
  Serial.println();
  Serial.println(__FUNCTION__);

  start = micros();
  for (uint16_t value = 0; value < MCP.maxValue(); value++)
  {
    x = MCP.analogWrite(value, 0);
  }
  stop = micros();
  Serial.print(MCP.maxValue());
  Serial.print(" x MCP.analogWrite():\t");
  Serial.print(stop - start);
  Serial.print("\t");
  Serial.println((stop - start) / (MCP.maxValue() + 1.0) );
  delay(1000);

  start = micros();
  for (uint16_t value = 0; value < MCP.maxValue(); value++)
  {
    MCP.fastWriteA(value);
    MCP.fastWriteB(MCP.maxValue()-value);
    delay(10);
  }
  stop = micros();
  Serial.print(MCP.maxValue());
  Serial.print(" x MCP.fastWriteA():\t");
  Serial.print(stop - start);
  Serial.print("\t");
  Serial.println((stop - start) / (MCP.maxValue() + 1.0) );
  delay(1000); 
}

bool last_a0_state = false;
bool triggerState = false;      // the current state of the output pin
bool buttonState = false;             // the current reading from the input pin
bool lastButtonState = LOW;

bool triggered = false;

unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 5;    // the debounce time; increase if the output flickers

unsigned long last_ticked = 0;

uint16_t var = 0;
void loop()
{
  /*performance_test();
  return;*/
  
  int areading = analogRead(IN_GATE); //>=768;
  //Serial.print("reading is ");
  //Serial.println(areading);
  
  bool reading = areading>=768;

  bool changed = false;
  
  // If the switch changed, due to noise or pressing:
  if (reading != last_a0_state) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      //Serial.print("changed to ");
      //Serial.println(reading);
      buttonState = reading;
      changed = true;

      // only toggle the LED if the new button state is HIGH
      if (buttonState == true) {
        triggered = true;
      } else {
        triggered = false;
      }   
    }
  }

  for (int i = 0 ; i < NUM_ENVELOPES ; i++) {
    float sustain = analogRead(IN_SUSTAIN)/1023.0f;
    if (sustain>envelopes[i].sustain_ratio+0.1 || sustain<envelopes[i].sustain_ratio-0.1) {
      Serial.print("setting\tsustain to\t");
      Serial.println(sustain);
      envelopes[i].sustain_ratio = sustain;
    }
    
    int release = map(analogRead(IN_RELEASE), 0, 1023, 0, 127);
    if (release>envelopes[i].release_length+1 || release<envelopes[i].release_length-1) {
      Serial.print("Setting\trelease to\t");
      Serial.println(release);
      envelopes[i].release_length = (release*(i+1));
    }
  }

  /*if (triggered) {
    //int a0 = analogRead(A0);
    int a1 = analogRead(A1);
    int a2 = analogRead(A2);
  
    var = map(a1, 0, 1024, 0, MCP.maxValue());
    MCP.fastWriteA(var);    
  
    var = map(a2, 0, 1024, 0, MCP.maxValue());
    MCP.fastWriteB(var);
  } else {
    MCP.fastWriteA(0);
    MCP.fastWriteB(0);
  }*/

  if (changed) {
    if (triggered) {
      Serial.println("==== Gate start");
      update_envelope(0, 127, true);
      //update_envelope(1, 127, true);
    } else {
      Serial.println("==== Gate stop");
      update_envelope(0, 0, false);
      //update_envelope(1, 0, false);
    }
  }
  if (bpm_clock()!=last_ticked) {
    process_envelopes(bpm_clock());
    last_ticked = bpm_clock();
  }
  
  //var++;
  //if (var>=MCP.maxValue()) var = 0;

  //delay(1);
  last_a0_state = reading;
}


// -- END OF FILE --
