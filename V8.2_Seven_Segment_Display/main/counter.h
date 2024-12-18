#ifndef COUNTER_H
#define COUNTER_H
#include <Arduino.h>  // Include Arduino functions like delay(), pinMode(), etc.

// Function prototypes (public)
void setupCounter();
void setOverrideMode(bool newState);

extern bool OVERRIDE_MODE;

class Counter {
  private:
    uint8_t numberOfDigits; // Number of digits
    uint8_t startingPosition; // The digit number that the counter starts at
    uint8_t* currentSequence; // Pointer for dynamic array
    bool validFlag; // a flag set that determins if the object is valid or not

    // Private class functions
    void displaySequence(uint8_t sequence, uint8_t digit);
    void shiftOutAll(uint8_t byteBlack, uint8_t byteWhite, uint8_t byteRelay);
    void resetShiftRegister();
    uint8_t calcMask(int dig);
    char reverseCase(char input);
    String repeatChar(char c, int n);
    void displayNum(int num, int dig);
    void displayChar(char letter, int dig);
    void clearDigit(int dig);
    void displayUnknown(char character, int pos);

  public:
    // Constructor and destructor
    Counter(uint8_t numDigits, uint8_t startingPos);
    ~Counter();

    // Public class functions
    void displayNumber(long number, char align = 'R');
    void displayWord(String word, char align = 'R');
    void clearDisplay(bool swipeFromR = true);
    void rollEffect();
    bool isValid();
};

#endif