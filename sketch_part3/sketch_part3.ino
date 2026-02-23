/*
PART 3
*/

// Shift Register Pin Assignments (PORTB)
#define SER_PIN   PB0 // Arduino D8  : SR1 SER (pin 14)
#define RCLK_PIN  PB3 // Arduino D11 : SR1/2 RCLK (pin 12)
#define SRCLK_PIN PB4 // Arduino D12 : SR1/2 SRCLK (pin 11)

// Globals for Interrupt
const int buttonPin = 2; // interrupt pin
volatile bool debounceLockout = false;
volatile unsigned long debounceStart = 0;
const unsigned long DEBOUNCE_MS = 200;

volatile uint8_t currentDigit = 0;


// Cols (cathodes) active LOW, on bits 0-4 (wired them 180 off, that's why they're flipped)
uint16_t colMask[5] = {
  (1 << 4), // col 1 (is bit 4)
  (1 << 3), // col 2
  (1 << 2), // col 3
  (1 << 1), // col 4
  (1 << 0)  // col 5
};

// Rows (anodes) active HIGH, on bits 5-11
uint16_t rowMask[7] = {
  (1 << 5),  // row 1 (top)
  (1 << 6),  // row 2
  (1 << 7),  // row 3
  (1 << 8),  // row 4
  (1 << 9),  // row 5
  (1 << 10), // row 6
  (1 << 11)  // row 7
};


// 5x7 Digit Font
const uint8_t DIGITS[10][7] = {
  // 0
  {
    0b01110,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b01110
  },
  // 1
  {
    0b00100,
    0b01100,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b01110
  },
  // 2
  {
    0b01110,
    0b10001,
    0b00001,
    0b00010,
    0b00100,
    0b01000,
    0b11111
  },
  // 3
  {
    0b01110,
    0b10001,
    0b00001,
    0b00110,
    0b00001,
    0b10001,
    0b01110
  },
  // 4
  {
    0b00010,
    0b00110,
    0b01010,
    0b10010,
    0b11111,
    0b00010,
    0b00010
  },
  // 5
  {
    0b11111,
    0b10000,
    0b11110,
    0b00001,
    0b00001,
    0b10001,
    0b01110
  },
  // 6
  {
    0b01110,
    0b10001,
    0b11110,
    0b10001,
    0b10001,
    0b10001,
    0b01110
  },
  // 7
  {
    0b11111,
    0b00001,
    0b00010,
    0b00100,
    0b01000,
    0b01000,
    0b01000
  },
  // 8
  {
    0b01110,
    0b10001,
    0b10001,
    0b01110,
    0b10001,
    0b10001,
    0b01110
  },
  // 9
  {
    0b01110,
    0b10001,
    0b10001,
    0b01111,
    0b00001,
    0b10001,
    0b01110
  }
};


// COUNTER

// Clock HELPERS
// Shift Clock: ea risiing edge shifts 1 new bit into the shift reg
inline void pulseSRCLK() {
  PORTB |=  (1 << SRCLK_PIN);
  PORTB &= ~(1 << SRCLK_PIN);
}
// Latch Clock: ea rising edge copies the 16'b internal shift reg to the output pins
inline void pulseRCLK() {
  PORTB |=  (1 << RCLK_PIN);
  PORTB &= ~(1 << RCLK_PIN);
}

// Shift out 16 bits (MSB first)
void shiftOut16(uint16_t data) {
  for (int i = 15; i >= 0; i--) {
    if (data & (1 << i))
      PORTB |=  (1 << SER_PIN);
    else
      PORTB &= ~(1 << SER_PIN);

    pulseSRCLK();
  }
  pulseRCLK();
}

// Render a single row of a digit once
void displayDigitOnce(uint8_t digit) {
  for (int row = 0; row < 7; row++) {
    uint8_t rowPattern = DIGITS[digit][row]; // which cols should be ON
    uint16_t out = 0;
    out |= rowMask[row]; // enable this row (anode HIGH)

    // Start with all column bits = 1 (OFF, bc active LOW)
    uint16_t colBits = 0;
    for (int c = 0; c < 5; c++) {
      // If this column should be ON for this row
      if (rowPattern & (1 << c)) {
        // Clear that column bit (drive cathode LOW)
        colBits |= 0; // we'll build via mask below
      }
    }

    // Build column bits: start all at 1, clear where needed
    uint16_t colMaskAll = (colMask[0] | colMask[1] | colMask[2] | colMask[3] | colMask[4]);
    uint16_t colOut = colMaskAll; // all OFF (1)
    for (int c = 0; c < 5; c++) {
      if (rowPattern & (1 << c)) {
        // Turn this column ON: clear its bit
        colOut &= ~colMask[c];
      }
    }

    out |= colOut;

    shiftOut16(out);

    // Short delay so this row is visible before we move to next
    delayMicroseconds(1000);
  }
}


// Interrup Handler
void buttonInterrupt() {
  volatile int buttonState = digitalRead(buttonPin);

  // FALLING edge press (button with pull-up means LOW = pressed)
  if (!debounceLockout && buttonState == LOW) {
    currentDigit = (currentDigit + 1) % 10;

    // Start debounce lockout
    debounceLockout = true;
    debounceStart = millis();

    // Disable new interrupts until debounce finishes
    detachInterrupt(digitalPinToInterrupt(buttonPin));
  }
}


// Setup
void setup() {
  // Configure SER, SRCLK, RCLK as outputs (PORTB bits)
  DDRB |= (1 << SER_PIN) | (1 << SRCLK_PIN) | (1 << RCLK_PIN);
  
  // Button on PD2 with internal pullup
  pinMode(buttonPin, INPUT_PULLUP);

  // Interrupt on FALLING
  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonInterrupt, FALLING);
}


// Main Loop: Cycle digits 0–9
void loop() {
  // LED matrix update
  displayDigitOnce(currentDigit);

  // Re-enable interrupt after debounce period
  if (debounceLockout && millis() - debounceStart >= DEBOUNCE_MS) {
    debounceLockout = false;
    attachInterrupt(digitalPinToInterrupt(buttonPin), buttonInterrupt, FALLING);
  }
}
