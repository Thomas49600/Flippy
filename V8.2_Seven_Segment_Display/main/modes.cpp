#include "modes.h"

// private functions
void addMode(JsonArray& modes, const char* arr1[], const char* arr2[][2]);

///////////////////////////// PUBLIC FUNCTIONS /////////////////////////////
void getModesJson(DynamicJsonDocument& doc) {
  // Populate the JSON document with the modes data
  JsonArray modes = doc.createNestedArray("modes");

  /*
    Example data for the first mode
    const char* arr1[3] = {"Timer", "OR", "3"};
    const char* arr2[3][2] = {{"=", "4"}, {"=", "6"}, {"=", "8"}};
    addMode(modes, arr1, arr2);
  */

  const char* arr2_0[1][2] = {{">", "0"}};  // any number of digits

  // Mode: Countdown
  const char* arr1_1[3] = {"Countdown", "AND", "2"};
  const char* arr2_1[2][2] = {{">", "2"}, {"<", "7"}};  // between 3 and 6 digits (no countdown can be over 2737.84 years)
  addMode(modes, arr1_1, arr2_1);

  // Mode: Dice
  const char* arr1_2[3] = {"Dice", "OR", "1"};
  addMode(modes, arr1_2, arr2_0);

  // Mode: Clock
  const char* arr1_3[3] = {"Clock", "OR", "2"};
  const char* arr2_3[2][2] = {{"=", "4"}, {"=", "5"}};  // 4 or 5 digits
  addMode(modes, arr1_3, arr2_3);

  // Mode: Counter
  const char* arr1_5[3] = {"Counter", "OR", "1"};
  addMode(modes, arr1_5, arr2_0);

  // Mode: Input
  const char* arr1_6[3] = {"Manual", "OR", "1"};
  addMode(modes, arr1_6, arr2_0);
}


//////////////////////////// PRIVATE FUNCTIONS ////////////////////////////
// This will add a mode to the JSON doc provided
/*
  You need to organise them as follows:
  arr1 (general info)  ->  [Name of the mode, logic for constraints, number of constraints (as a char array)]
  arr2 (Constraints)   ->  [[logic opperator, numerical value (as a char)], ...] // Corresponding number of arrays to the third value in arr1
*/
void addMode(JsonArray& modes, const char* arr1[], const char* arr2[][2]) {
  JsonObject newMode = modes.createNestedObject();
  newMode["name"] = arr1[0];
  newMode["logic"] = arr1[1];

  JsonArray constraints = newMode.createNestedArray("constraints");
  int numConstraints = atoi(arr1[2]);  // Converts the string "2" to the integer 2

  for (int i = 0; i < numConstraints; i++) { // for each constraint
    JsonObject newConstraint = constraints.createNestedObject();
    newConstraint["operator"] = arr2[i][0];
    newConstraint["value"] = atoi(arr2[i][1]);
  }
}