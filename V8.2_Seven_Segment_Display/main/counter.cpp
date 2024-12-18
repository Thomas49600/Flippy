#include "counter.h"

#define D0_GPIO 16  // Corresponds to D0 pin
#define D1_GPIO 5   // Corresponds to D1 pin
#define D2_GPIO 4   // Corresponds to D2 pin
#define D3_GPIO 0   // Corresponds to D3 pin
#define D4_GPIO 2   // Corresponds to D4 pin
//#define D5_GPIO 14  // Corresponds to D5 pin
//#define D6_GPIO 12  // Corresponds to D6 pin
//#define D7_GPIO 13  // Corresponds to D7 pin
//#define D8_GPIO 15  // Corresponds to D8 pin

const uint8_t latchPin = D2_GPIO; // STCP or RCLK pin D2
const uint8_t clockPin = D0_GPIO; // SHCP or SRCLK pin D0
const uint8_t dataPin = D4_GPIO; // DS or SER pin D4

const uint8_t enablePin = D3_GPIO; // OE pin D3 - only used for complex control
//const uint8_t resetPin = D1_GPIO; // CLEAR pin D1 - only used for complex control  

#define FLAP_TIME 50 // <=20 is too short of a pulse for the top or bottom digit to push the flap up
#define UP_MID_DELAY 45 // a bit of an extra time for the middle flap when pushing upwards, this is needed to avoid it 'bouncing' due to it moving much faster
#define UP_TB_DELAY 40 // same as the UP_MID_DELAY but for the top and bottom flap


/*
Define the position of white for each number 0 - 9 and blank (can also add letters etc if wanted)
Where {a, b, c, d, e, f, g, dp} corresponds to:

    --- a ---
  |           |
  f           b
  |           |
    --- g --- 
  |           |
  e           c
  |           |
    --- d ---    (dp)  -> Note dp can be a comma, colon or any other segment
*/

//white is 1, black is 0
const uint8_t digitValues[] = {
  0b11111100, // 0
  0b01100000, // 1
  0b11011010, // 2
  0b11110010, // 3
  0b01100110, // 4
  0b10110110, // 5
  0b10111110, // 6
  0b11100000, // 7
  0b11111110, // 8
  0b11110110  // 9
};

const char letterKeys[] = {'A', 'b', 'C', 'c', 'd', 'E', 'F', 'G', 'H', 'h', 'I', 'J', 'L', 'N', 'n', 'O', 'o', 'P', 'q', 'r', 'S', 't', 'U', 'u', 'y', '-'};
const uint8_t letterKeysSize = sizeof(letterKeys) / sizeof(letterKeys[0]); // total number of BITS in the array, divided by the number of bits 1 charactor has
const uint8_t letterValues[] = {
  0b11101110, // A
  0b00111110, // b
  0b10011100, // C
  0b00011010, // c
  0b01111010, // d
  0b10011110, // E
  0b10001110, // F
  0b10111100, // G
  0b01101110, // H
  0b00101110, // h
  0b01100000, // I
  0b01110000, // J
  0b00011100, // L
  0b11101100, // N
  0b00101010, // n
  0b11111100, // O
  0b00111010, // o
  0b11001110, // P
  0b11100110, // q
  0b00001010, // r
  0b10110110, // S
  0b00011110, // t
  0b01111100, // U
  0b00111000, // u
  0b01110110, // y
  0b00000010  // -
};

const uint8_t BLANK = 0b00000000; // Set all to black

// The below can be set to true or false
// It determins if the flaps are in an override mode (where even if it is white showing, it still tries to show white again)
// Or not in override mode (where if a flap is white and it wants it to be white, it just skips over it)
// Sometimes flaps may get stuck or 'bouce' meaning they're  not in the expected state, override fixes this but is slower for general performance

// Define the global variable
bool OVERRIDE_MODE = true;  // Initial value

#define TOTAL_DIGITS 8 // the total possible number of digits, unless you build an expansion board you can't exceed 8 (nothing is designed to go past 8 digits as I don't expect anyone to ever make that many)

///////////////////////////// PUBLIC FUNCTIONS /////////////////////////////
void setupCounter() {
  // Setup pins as Outputs
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  pinMode(enablePin, OUTPUT);
  //pinMode(resetPin, OUTPUT);

  // establish normal operation mode
  digitalWrite(enablePin, LOW);
  //digitalWrite(resetPin, HIGH);

  delay(100); // wait one sec as otherwise you have random issues

  // Make sure all shift regesters are set to clear.
  shiftOut(dataPin, clockPin, LSBFIRST, 0x00);
  shiftOut(dataPin, clockPin, LSBFIRST, 0x00);
  shiftOut(dataPin, clockPin, LSBFIRST, 0x00);
}

// Setter function for OVERRIDE_MODE
void setOverrideMode(bool newState) {
  OVERRIDE_MODE = newState;  // Set the value of OVERRIDE_MODE to newState
}

///////////////////////////// Counter FUNCTIONS /////////////////////////////
// CONSTRUCTOR & DESTRUCTOR
Counter::Counter(uint8_t numDigits, uint8_t startingPos) {
  validFlag = false; // assume it is invalid
  currentSequence = nullptr;  // Set the pointer to nullptr by default

  if (numDigits > 0) { // can't have negative digits
    numberOfDigits = numDigits;

    if (startingPos >= 0) { // check that the position is at least the 1st one
      if ((startingPos + numDigits - 1) <= TOTAL_DIGITS) { //check that the counter doesn't exceed the total number of digits (when shifted by the startingPos)
        startingPosition = startingPos;
        validFlag = true; // set the flag as true as it passed all checks
      
      } else {
      Serial.println("Digit overflow");
      }

    } else {
      Serial.println("Invalid starting position");
    }

  } else {
    Serial.println("Must have at least 1 digit");
  }

  
  if (validFlag == true) { // if it is valid, create the currentSequence array and clear the display
    currentSequence = new uint8_t[numberOfDigits](); // Allocate memory for the array
    
    // set all values to 255, this ensures all flaps are initally reset
    for (int i = 0; i < numberOfDigits; ++i) {
      currentSequence[i] = 255; // i.e. B11111111
    }
    
    clearDisplay(false); // will swipe from L - R
  }
}

Counter::~Counter() { // destructor
  if (currentSequence != nullptr) { // Check if the pointer is not null
    delete[] currentSequence;  // Safely delete the allocated memory
  }
}


// PRIVATE CLASS FUNCTIONS
// the sequence should be a byte where 1 = white/colour and 0 = black
// digit should be the digit to apply this charactor too, NOT formatted as a byte
void Counter::displaySequence(uint8_t sequence, uint8_t digit) {
  uint8_t white; // for storing the values we want to send to the shift regesters
  uint8_t black;
  uint8_t byteMask = currentSequence[digit-1] ^ sequence;  // preform an XOR calc to find the new bits

  // The mask defines what digit to control
  uint8_t mask = calcMask(digit); // formats it as a byte

  // loop through every bit of the byte
  for (int i = 7; i >= 0; i--) {
    // for each location, if we are overriding, or if we aren't overriding but the byteMask tells us this flap needs to change
    if (OVERRIDE_MODE || (!OVERRIDE_MODE && (bool((byteMask >> i) & 1)))) {  
      white = 0b00000000; // reset them every time
      black = 0b00000000;

      bool isOne = bool((sequence >> i) & 1);  // Right shift the byte by 'i' bits and AND with 1 (1 = 0b00000001) -> then convert to a boolean
      // this checks if the bit is 1 starting from the LHS -> RHS of the byte

      if (isOne) { // if this number in the sequence is a one
        // we need to turn the flap white
        white = 1 << i;

      } else {
        // we want the flap to turn black
        black = 1 << i;
      }

      // now that we have the correct sequence to send as a command to the digit
      shiftOutAll(black, white, mask);
    }
  }

  currentSequence[digit-1] = sequence; // update the current sequence status
}

void Counter::shiftOutAll(uint8_t byteBlack, uint8_t byteWhite, uint8_t byteRelay) {
  // since some counters will have a different starting position, we need to shift the byteRelay accordinly
  uint8_t shiftedByteRelay = byteRelay >> (startingPosition - 1);

  digitalWrite(latchPin, LOW);  // ST_CP LOW to keep flaps from changing while reading serial data

  // You must shift them out in reverse order
  shiftOut(dataPin, clockPin, LSBFIRST, shiftedByteRelay);
  shiftOut(dataPin, clockPin, LSBFIRST, byteWhite);
  shiftOut(dataPin, clockPin, LSBFIRST, byteBlack);

  digitalWrite(latchPin, HIGH);  // ST_CP HIGH change the flaps

  if (bool((byteWhite >> 1) & 1)) {
    // if we are flipping the middle segment UP (white), we need to delay slightly longer to avoid it bouncing
    delay(FLAP_TIME + UP_MID_DELAY);

  } else if (bool((byteBlack >> 7) & 1) || bool((byteWhite >> 4) & 1)) {
    // if we are flipping the top segment UP (Black) or the bottom segment UP (White), we need to delay slightly longer to avoid it bouncing
    delay(FLAP_TIME + UP_TB_DELAY);
  
  } else {
    delay(FLAP_TIME); // wait for flaps to flip
  }

  resetShiftRegister(); // reset them all to 0 for the next command
  
  /*
  Serial.print("Relay: ");
  Serial.println(shiftedByteRelay, BIN);  // Print the binary value of shiftedByteRelay
  Serial.print(" | White: ");
  Serial.print(byteWhite, BIN);  // Print the binary value of byteWhite
  Serial.print(" | Black: ");
  Serial.println(byteBlack, BIN);  // Print the binary value of byteBlack and add a newline at the end
  */
}

// sets all shift regesters to store 0x00 (0b00000000)
void Counter::resetShiftRegister() {
  /*
  digitalWrite(resetPin, LOW);   // Pull MR pin LOW to reset
  delay(10);                     // Wait for a short moment
  digitalWrite(resetPin, HIGH);  // Return MR pin HIGH (normal operation)
  */

  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, LSBFIRST, 0x00);
  shiftOut(dataPin, clockPin, LSBFIRST, 0x00);
  shiftOut(dataPin, clockPin, LSBFIRST, 0x00);
  digitalWrite(latchPin, HIGH);
}

// used to calculate the squence for a digit
uint8_t Counter::calcMask(int dig) {
  // The mask defines what digit to control
  // If the code controls the right most digit as digit '1' then change (128 >>) to (1 <<) || i.e [1][2][3] (what it currently is) v.s. [3][2][1]
  uint8_t mask = 128 >> (dig - 1);
  return mask;
}

// used to convert lowercases to upper and vise-versa
char Counter::reverseCase(char input) {
  if (input >= 'A' && input <= 'Z') { // compares it to their ASCII values
    // It's an uppercase letter, convert to lowercase
    return input + 32; // the ASCII value of captial and lowercase letters differs by 32

  } else if (input >= 'a' && input <= 'z') {
    // It's a lowercase letter, convert to uppercase
    return input - 32;
  }

  // If it's not a letter, return the input unchanged (fail-safe)
  return input;
}

// Function to repeat a character 'n' times
String Counter::repeatChar(char c, int n) {
    String result = "";
    for (int i = 0; i < n; i++) {
        result += c;
    }
    return result;
}

// Expects a number between 0-9 for num, and a int representative of which digit to assign it too (starting at 1 for the right most digit)
void Counter::displayNum(int num, int dig) {  
  uint8_t sequence = digitValues[num];
  displaySequence(sequence, dig);
}

// Expects a character, and a int representative of which digit to assign it too (starting at 1 for the right most digit)
void Counter::displayChar(char letter, int dig) {
  uint8_t index;
  uint8_t sequence;
  bool foundLetter = false;

  // only want to check twice (once in orginal case, once flipped)
  for (uint8_t i = 0; i < 2; i++) {
    // check for an exact match first, then one with reversed case
    for (uint8_t j = 0; j < letterKeysSize; j++) {
      if (letterKeys[j] == letter) {
        index = j;
        foundLetter = true;
        break;
      }
    }

    if (foundLetter) {
      break; // don't search again
    
    } else {
    letter = reverseCase(letter); // reverse the case and check again
    }
  }

  if (foundLetter) {
    sequence = letterValues[index]; // since we know what position the letter was in, we know what position the byte is in (a makeshift dictionary.)

  } else {
    Serial.println("This letter cannot be printed");
    sequence = BLANK;
  }

  displaySequence(sequence, dig);
}

// Clears a digit (sets all to black)
void Counter::clearDigit(int dig) {
  displaySequence(BLANK, dig);
}

void Counter::displayUnknown(char character, int pos) {
  if (character == '#') { // the hash is a placeholder for clear, this way it doesn't need to check everything else first
    clearDigit(pos);

  } else if (character >= '0' && character <= '9') { // if the char is a number
    int currentInt = character - '0'; // Converts char '5' to int 5 etc.
    displayNum(currentInt, pos); // display the integer to the correct digit

  } else if ((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') || (character == '-')) { // if the char is a letter OR -
    displayChar(character, pos); // display the character to the correct digit
    // if we cannot display the charactor, it will clear the digit (as part of displayChar)

  } else {
    clearDigit(pos); // if it is an invalid character just clear that digit
  }
}


// PUBLIC CLASS FUNCTIONS
// Expects a number
void Counter::displayNumber(long number, char align) {  
  // if you wanted to print numbers in a different way to words (e.g. leading zeros etc) you could do that here
  displayWord(String(number), align);
}

// Expects a word (numbers +/- letters) and will print it accordingly to it's align parameter
void Counter::displayWord(String word, char align) {
  int length = word.length();
  
  if (align == 'R') {
    for (int i = numberOfDigits; i > 0; i--) {
      // Determine the index for the current character
      int charIndex = length - i;

      if (charIndex < 0) { // If the index is negative, it means word is too short
        clearDigit(i); // Clear the digit for position i

      } else {
        char currentChar = word.charAt(charIndex); // Get the current character
        displayUnknown(currentChar, i); // display the charactor without knowing if it is a letter or number        
      }
    }

  } else if (align == 'L') {
    for (int i = numberOfDigits; i > 0; i--) {
      // Calculate the character index for the current position
      int charIndex = numberOfDigits - i; // Start from the first character for the first position

      if (charIndex < length) { // If there's still a character to display
        char currentChar = word.charAt(charIndex); // Get the current character
        displayUnknown(currentChar, i); // display the charactor without knowing if it is a letter or number
      
      } else { // If we exceed the length of the word, clear this position
        clearDigit(i);
      }
    }
  
  } else if (align == 'C') {
    int totalSpaces = numberOfDigits - length;
    
    if (totalSpaces <= 0) {
      displayWord(word, 'R'); // if we can't centre it, or there is no need to centre it as it matches the correct length, just display it normally
    
    } else {
      int leftSpaces = totalSpaces/2; // this will strip decimals, meaning if it is uneven, the left will have one less space
      int rightSpaces = totalSpaces - leftSpaces;

      String output = repeatChar('#', leftSpaces) + word + repeatChar('#', rightSpaces); // # is a way of telling displayUnknown to clear
      //Serial.println(output);
      displayWord(output, 'R'); // display the word, now with the correct length so we can align R
    }

  } else { // if align is not in [R, L or C], print an error
    displayWord("Error", 'L');
    Serial.print(align); Serial.println(" is not a valid align.");
  }
}

// Will clear the entire display, from either R -> L (swipeFromR = true; default) or L -> R (swipeFromR = false)
void Counter::clearDisplay(bool swipeFromR) {
  if (swipeFromR) {
    for (int i = 1; i <= numberOfDigits; i++) {
      clearDigit(i);
    }

  } else {
    for (int i = numberOfDigits; i >= 1; i--) {
      clearDigit(i);
    }
  }
}

// used to flap randomly to give the effect of rolling a dice
void Counter::rollEffect() {
  // Idea 1: Do like a 'strobe' around the outside (clear -> stobe)
  // Idea 2: Do completely random flaps (random num 1-7, random dig 1-X, toggle Wt/Bk)
  // Idea 3: Do a 'strobe' around the outside of each digit (clear -> essentially a 0 but it starts clearing as some others are flapping)

  // Doing idea 3
  for (int i = numberOfDigits; i > 0; i--) { // for each digit (L -> R)
    const uint8_t biteRelay = calcMask(i);
    uint8_t biteWhite = 0b10000000;  // Start biteWhite at 10000000
    uint8_t biteDelay = 0b00000000;  // Set a 1 flap delay
    uint8_t biteBlack = 0b00000000;  // Start biteBlack at 00000000

    for (int i = 0; i < 6; i++) { // loop 6 times
      shiftOutAll(biteBlack, biteWhite, biteRelay);
      
      biteBlack = biteDelay; // this progresses biteBlack forward each time but 'chasing' biteWhite
      biteDelay = biteWhite; // keep an extra 1 flap gap between the front of the 'strobe' (so it is 2 flaps long)
      biteWhite = biteWhite >> 1; // shift byte white along by 1
    }

    shiftOutAll(0b00001000, 0b00000000, biteRelay);
    shiftOutAll(0b00000100, 0b00000000, biteRelay);
    shiftOutAll(0b00000010, 0b00000000, biteRelay);
  }
}

// this function should be called immidatly after construction to ensure the object is valid and hence only assign it memory if it is (e.g. in an array)
bool Counter::isValid() {
  return validFlag; // this acts as a getter to prevent modification to the flag publicly
}

///////////////////////////// END OF Counter FUNCTIONS /////////////////////////////



