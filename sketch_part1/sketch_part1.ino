/*
PART 1
Concept: Only accept the new state if the raw signals has stayed the same for at least 'DEBOUNCE' ms
1. Intialize variables
2. Helper to read raw INput
3. Loop has logic to debounce and display current state of the switch to the terminal
*/
const unsigned long DEBOUNCE = 30;

bool stable_state = false; // debounced logical state
bool last_raw_state = false; // last raw reading
unsigned long last_change_time = 0; // when the raw reading last changed

void setup() {
  Serial.begin(9600);

  // Configure PD2 (button press --> PD2 HI, vv)
  // DDRD bit 2 = 0 --> PD2 is input
  DDRD &= ~(1 << DDD2);
  // PORTD bit 2 = 1 --> enable internal pull-up on PD2 IN
  PORTD |= (1 << PORTD2);
}

// Ret TRUE when button is pressed
bool read_button_raw() {
  // PIND bit 2: Actual voltage on PD2
  return (PIND & (1 << PIND2)) == 0;
}


void loop() {
  // 1. Read raw noisy INput
  bool raw = read_button_raw(); // inst. noisy reading from pin
  unsigned long now = millis();

  // a. Raw reading changed, reject and reset (ie. noisy)
  if (raw != last_raw_state) {
    last_raw_state = raw; // update last raw reading var
    last_change_time = now; // reset timer
  }

  // b. If the raw state has been stable for DEBOUNCE, accept it
  if ((now - last_change_time) >= DEBOUNCE) {
    // Debounced state changes
    if (raw != stable_state) {
      stable_state = raw;

      if (stable_state) {
        Serial.println("STATE: Pressed");
      } else {
        Serial.println("STATE: Released");
      }
    }
  }

  delay(1);
}
