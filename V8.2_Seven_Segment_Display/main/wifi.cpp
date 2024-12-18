#include "wifi.h"
#include "modes.h"

#include <ArduinoJson.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h> 
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>   // Include the WebServer library
#include <LittleFS.h>           // Include the LittleFS library

// for real time
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <time.h>

// Set up the WiFi and NTP client
WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org", 0, 60000); // Update every minute (set to UTC +00:00)
unsigned long previousMillis = 0;  // Stores last update time
unsigned long currentMillis = 0;   // Stores current time
unsigned long nextUpdateMillis = 0; // Stores the time when the next update should happen

#define MAX_COUNTERS 8 // max number of possible counters
const char* mdnsName = "flippy"; // Domain name for the mDNS responder

ESP8266WiFiMulti wifiMulti;     // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'

ESP8266WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80
WebSocketsServer webSocket(81);    // create a websocket server on port 81

// private functions
void setupModes();
void handleDisplay();
void handleClockAPI();
void refreshClockStates(bool justClear=false);
void updateClockDisplays();
void refreshCountdownStates(bool justClear=false);
void updateCountdownDisplays();
void resizeArrays(int newSize);
void updateCounterObjects(JsonArray groups);
void saveDataConfig(const DynamicJsonDocument& doc_new);
uint8_t saveDataSavedModes(const DynamicJsonDocument& doc_new, bool clockAPICalling);
void saveDataReset();
void displayData(const DynamicJsonDocument& doc, bool messageServer=true);
void loadData(int client, const DynamicJsonDocument& doc);
void saveJsonToFile(const DynamicJsonDocument& doc);
DynamicJsonDocument loadJsonFromFile();
static String getContentType(String filename);
static bool handleFileRead(String path);
void freeCounterObjMemory();

// Define the global object pointers
Counter** counterObjs = nullptr; // Pointer to hold the dynamic array of Counter objects pointers
int* counterObjGroupIDs = nullptr; // Pointer to hold the dynamic array of integers
int sizeOfObjectArray = 0; // Initialize size

long clockIdArray[2][3] = {{0, 0, 0}, {0, 0, 0}}; // with 8 digits you can have a max of 2 clocks (4 & 4) - {groupID, format, timeOffset}

long countdownIdArray[2][2] = {{0, 0}, {0, 0}}; // with 8 digits you can have a max of 2 countdowns (3 & 3 with 2 left over) - {groupID, timeOffset}
char countdownTargetArray[2][17] = {"yyyy-mm-ddThh:mm", "yyyy-mm-ddThh:mm"};
String oldCountdownDisplay0 = "none";
String oldCountdownDisplay1 = "none";

///////////////////////////// PUBLIC FUNCTIONS /////////////////////////////
void setupWifi() {
  wifiMulti.addAP("wifi ssid", "password");   // add Wi-Fi networks you want to connect to
  // wifiMulti.addAP("wifi ssid 2", "password 2");

  Serial.println();
  Serial.println("Connecting ...");
  int i = 0;
  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
    delay(250);
    Serial.print('.');
  }
  
  Serial.println('\n');
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());              // Tell us what network we're connected to
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());           // Send the IP address of the ESP8266 to the computer

  if (MDNS.begin(mdnsName)) {              // Start the mDNS responder for esp8266.local
    Serial.print("mDNS responder started: http://"); Serial.print(mdnsName); Serial.println(".local");
  } else {
    Serial.println("Error setting up MDNS responder!");
  }  

  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  Serial.println("WebSocket server started.");

  LittleFS.begin();                           // Start the SPI Flash Files System

  server.on("/esp-apis/display", HTTP_POST, handleDisplay); // Define the /display API route and the handler function
  server.on("/esp-apis/clock", HTTP_POST, handleClockAPI); // Define the /clock API route and the handler function
  server.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'

  server.begin();                           // Actually start the server
  Serial.println("HTTP server started");

  setupModes(); // setup the modes

  // Start the NTP Client to get time
  timeClient.begin();

  // Calculate the time difference from the current time to the next whole minute
  timeClient.update();
  int rawMillis = (timeClient.getSeconds())*1000;
  currentMillis = millis();
  nextUpdateMillis = (60000 - rawMillis) + currentMillis + 1000; // Time to next minute (full min - amount we are already through) + current real time + an extra second so we don't get the old time
}

void setupCounterObjs() {
  DynamicJsonDocument doc = loadJsonFromFile(); // load the json
  updateCounterObjects(doc["configData"]["groups"]);
  setOverrideMode(doc["configData"]["override"].as<bool>());

  refreshClockStates();
  refreshCountdownStates();
}

void wifiChecks() {
  currentMillis = millis();

  if (currentMillis >= nextUpdateMillis) {  // If it's time for the next update
    previousMillis = currentMillis;  // Update previousMillis to the current time
    
    timeClient.update();
    nextUpdateMillis = (60000 - (timeClient.getSeconds())*1000) + millis() + 1000; // Time to next minute (full min - amount we are already through) + current real time + an extra second so we don't get the old time

    // Use this to update all time based functions... clocks, countdowns etc.
    updateClockDisplays();
    updateCountdownDisplays();
  }

  server.handleClient();                    // Listen for HTTP requests from clients
  MDNS.update();
  webSocket.loop();                         // constantly check for websocket events
  timeClient.update();
}

void handleNotFound(){ // if the requested file or page doesn't exist, return a 404 not found error
  if(!handleFileRead(server.uri())){          // check if the file exists in the flash memory (LittleFS), if so, send it
    server.send(404, "text/plain", "404: File Not Found");
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  if (type == WStype_TEXT) { // if new text data is received
    Serial.printf("[%u] get Text: %s\n", num, payload);

    DynamicJsonDocument doc(1024);  // Allocate a buffer for the JSON document
    DeserializationError error = deserializeJson(doc, payload);  // Parse the JSON string (payload is a char array)
    
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    int messageType = doc["type"].as<int>();  // Convert JSON value to int
    switch (messageType) {
      case 0: // config data
        saveDataConfig(doc);
        refreshClockStates(true);
        refreshCountdownStates(true);
        break;

      case 1: // saved mode
        saveDataSavedModes(doc, false);
        break;

      case 2: // reset flag
        saveDataReset();
        refreshClockStates(true);
        refreshCountdownStates(true);
        break;

      case 3: // reqest data
        loadData(num, doc); // num tells it who asked for it and thus who to reply to
        break;

      default:
        Serial.print("Unknown typeID: ");
        Serial.println(messageType);
        break;
    }
  }
}


///////////////////////////// PRIVATE FUNCTIONS /////////////////////////////
void setupModes() {
  DynamicJsonDocument modeJson(512);
  getModesJson(modeJson);

  //serializeJsonPretty(modeJson, Serial);

  DynamicJsonDocument doc(1024);
  doc = loadJsonFromFile();
  
  doc["modes"] = modeJson["modes"]; // override the current possible modes

  saveJsonToFile(doc); // then save it for the remainder of opperations
}

void handleDisplay() {
  String body = server.arg("plain"); // Get the request body
  
  DynamicJsonDocument doc(256);    
  DeserializationError error = deserializeJson(doc, body); // Parse the JSON body
  
  if (error) {
    server.send(400, "text/plain", "Invalid JSON"); // If parsing fails, return an error as plain text
    return;
  }

  // Extract values from JSON using keys
  if (doc.containsKey("groupID") && doc.containsKey("content") && doc.containsKey("type") && doc.containsKey("align")) {
    displayData(doc); // send the 200 response in this function
    
  } else {
    server.send(400, "text/plain", "Missing keys in JSON"); // If keys are missing, send a 400 Bad Request error as plain text
  }
}

void handleClockAPI() {
  String body = server.arg("plain"); // Get the request body
  
  DynamicJsonDocument doc(256);    
  DeserializationError error = deserializeJson(doc, body); // Parse the JSON body
  
  if (error) {
    server.send(400, "text/plain", "Invalid JSON"); // If parsing fails, return an error as plain text
    return;
  }

  // Extract values from JSON using keys
  if (doc.containsKey("savedModeID") && doc.containsKey("groupID") && doc.containsKey("state")) {
    uint8_t successNum = saveDataSavedModes(doc, true);
    
    switch (successNum) {
      case 0: server.send(200, "text/plain", "State updated"); break;
      case 1: server.send(400, "text/plain", "No matching saved mode was found"); break; // User needs to re-save config page
      case 2: server.send(400, "text/plain", "The matching saved mode had no data already stored"); break; // User needs to save clock data
    }
    
  } else {
    server.send(400, "text/plain", "Missing keys in JSON"); // If keys are missing, send a 400 Bad Request error as plain text
  }
}

void refreshClockStates(bool justClear) {
  for (int i = 0; i < 2; i++) { // reset the current values to 0
    for (int j = 0; j < 3; j++) {
        clockIdArray[i][j] = 0;
    }
  }

  if (!justClear) {
    DynamicJsonDocument doc = loadJsonFromFile(); // get the current json
    int idsToAdd[2]; // to hold the savedModeIDs if we find any
    int index = 0;

    for (JsonObject mode : doc["savedModes"].as<JsonArray>()) {
      if (mode["name"] == "Clock" && mode.containsKey("data")) { // if it is a clock and has saved data
        if (mode["data"].containsKey("state")) { // if it has the state key
          if (mode["data"]["state"] == "on") { // and the state is on
            idsToAdd[index] = mode["ID"].as<int>();
            clockIdArray[index][1] = mode["data"]["format"].as<long>();
            clockIdArray[index][2] = mode["data"]["timezoneOffset"].as<long>();
            index += 1;
          }
        }
      }
    }

    for (int i = 0; i < index; i++) { // for all the saved mode id's we found
      for (JsonObject group : doc["configData"]["groups"].as<JsonArray>()) { // for each saved group
        if (group["savedModeID"] == idsToAdd[i]) { // if it is the matching group
          clockIdArray[i][0] = group["ID"].as<long>(); // add this group ID to the clocks we need to update
          break; // stop looking (and move onto the next one)
        }
      }
    }

    // then immidiatly display to the clock
    updateClockDisplays();
  }
}

void updateClockDisplays() {
  for (int i = 0; i < 2; i++) {
    if (clockIdArray[i][0] != 0) { // if we have clocks that are on (aka some saved groupIDs)
      timeClient.setTimeOffset(clockIdArray[i][2]); // in seconds
      int rawHours = timeClient.getHours();
      int rawMinutes = timeClient.getMinutes();

      String period;
      if (clockIdArray[i][1] == 12) { // 12h time (we need to modify the formatting)
        period = "A";
        if (rawHours >= 12) {
          period = "P";
          if (rawHours > 12) rawHours -= 12; // Convert hours from 24-hour to 12-hour
        }
        if (rawHours == 0) rawHours = 12; // Handle midnight as 12 AM
      }

      String formattedTime;
      if (rawMinutes < 10) {
        formattedTime = String(rawHours) + "0" + String(rawMinutes); // add leading 0
      } else {
        formattedTime = String(rawHours) + String(rawMinutes); // format the hours and minutes
      }


      if ((String(clockIdArray[i][0]).length() == 5) && (clockIdArray[i][1] == 12)) { // if we have 5 digits (the clock format must have been 12 but just to be sure)
        formattedTime = formattedTime + period; // add the letter A or P
      }

      DynamicJsonDocument doc(256);
      // Fill the JSON document with the given key-value pairs
      doc["groupID"] = clockIdArray[i][0];
      doc["content"] = formattedTime;
      doc["type"] = "text";
      doc["align"] = "R";

      displayData(doc, false);
    }
  }
}

void refreshCountdownStates(bool justClear) {
  for (int i = 0; i < 2; i++) { // reset the current values
    for (int j = 0; j < 2; j++) {
        countdownIdArray[i][j] = 0;
    }
    strcpy(countdownTargetArray[i], "yyyy-mm-ddThh:mm");
  }

  oldCountdownDisplay0 = "none";
  oldCountdownDisplay1 = "none";

  if (!justClear) {
    DynamicJsonDocument doc = loadJsonFromFile(); // get the current json
    int idsToAdd[2]; // to hold the savedModeIDs if we find any
    int index = 0;

    for (JsonObject mode : doc["savedModes"].as<JsonArray>()) {
      if (mode["name"] == "Countdown" && mode.containsKey("data")) { // if it is a clock and has saved data
        if (mode["data"].containsKey("state")) { // if it has the state key
          if (mode["data"]["state"] == "on") { // and the state is on
            idsToAdd[index] = mode["ID"].as<int>();
            countdownIdArray[index][1] = mode["data"]["timezoneOffset"].as<long>();

            String target = mode["data"]["target"].as<String>();  // Get the String from the JSON
            target.toCharArray(countdownTargetArray[index], 17);   // Copy the String into the char array
            index += 1;
          }
        }
      }
    }

    for (int i = 0; i < index; i++) { // for all the saved mode id's we found
      for (JsonObject group : doc["configData"]["groups"].as<JsonArray>()) { // for each saved group
        if (group["savedModeID"] == idsToAdd[i]) { // if it is the matching group
          countdownIdArray[i][0] = group["ID"].as<long>(); // add this group ID to the clocks we need to update
          break; // stop looking (and move onto the next one)
        }
      }
    }

    // then immidiatly display to the countdown
    updateCountdownDisplays();
  }
}

void updateCountdownDisplays() {
  for (int i = 0; i < 2; i++) {
    if (countdownIdArray[i][0] != 0) { // if we have countdowns that are on (aka some saved groupIDs)
      // Get all the target times
      String targetString = countdownTargetArray[i];
      //Serial.println(targetString);

      struct tm timeinfo;
      timeinfo.tm_year = targetString.substring(0, 4).toInt() - 1900; // Year since 1900
      timeinfo.tm_mon = targetString.substring(5, 7).toInt() - 1; // Month (0-11)
      timeinfo.tm_mday = targetString.substring(8, 10).toInt();
      timeinfo.tm_hour = targetString.substring(11, 13).toInt();
      timeinfo.tm_min = targetString.substring(14, 16).toInt();
      timeinfo.tm_sec = 0;

      timeClient.setTimeOffset(countdownIdArray[i][1]); // set timezone
      unsigned long currentEpoch = timeClient.getEpochTime(); // Get current time in seconds since epoch (Unix timestamp)
      unsigned long targetEpoch = mktime(&timeinfo);  // Convert target date into Unix timestamp

      String formattedDisplay;
      if (currentEpoch >= targetEpoch) { // countdown complete
        formattedDisplay = 0;
      
      } else {
        unsigned long secondsRemaining = targetEpoch - currentEpoch;

        //Serial.println(currentEpoch);
        //Serial.println(targetEpoch);
        //Serial.println(secondsRemaining);

        float daysRemaining = secondsRemaining / 86400.0;
        if (daysRemaining > 1) {
          Serial.print("D"); Serial.println(daysRemaining);
          formattedDisplay = String(int(daysRemaining)) + "d";

        } else {
          float hoursRemaining = secondsRemaining / 3600.0;
          if (hoursRemaining > 1) {
            Serial.print("H"); Serial.println(hoursRemaining);
            formattedDisplay = String(int(hoursRemaining)) + "h";
          
          } else {
            float minutesRemaining = secondsRemaining / 60.0;
            Serial.print("M"); Serial.println(minutesRemaining);
            formattedDisplay = String(int(minutesRemaining)+1); // this +1 means that it doesn't say the time is up at 0.9 minutes left

            // once the time is > or = target time, it will default to 0
          }
        }
      }

      Serial.println(formattedDisplay);
      Serial.println(oldCountdownDisplay0);
      //Serial.println(oldCountdownDisplay1);
      //Serial.println(i);

      bool update = false;
      if (i == 0 && formattedDisplay != oldCountdownDisplay0) { // if it is the first counter, and it needs to be changed
        oldCountdownDisplay0 = formattedDisplay;
        update = true;
      
      } else if (i == 1 && formattedDisplay != oldCountdownDisplay1) { // if it is the second counter, and it needs to be changed
        oldCountdownDisplay1 = formattedDisplay;
        update = true;
      }

      if (update) {
        DynamicJsonDocument doc(256);
        // Fill the JSON document with the given key-value pairs
        doc["groupID"] = countdownIdArray[i][0];
        doc["content"] = formattedDisplay;
        doc["type"] = "text";
        doc["align"] = "R";

        if (formattedDisplay == "0") {
          doc["mode"] = "Dice"; // if we just hit 0, play the dice animation as a celebration
        }

        displayData(doc, false);
      }
    }
  }
}

void resizeArrays(int newSize) {
  if (newSize > 0 && newSize <= MAX_COUNTERS) {
    freeCounterObjMemory(); // Free previously allocated memory

    sizeOfObjectArray = newSize; // Set the new size

    // Allocate new memory for both arrays
    counterObjs = new Counter*[sizeOfObjectArray]; // Allocate array of pointers to Counter objects
    counterObjGroupIDs = new int[sizeOfObjectArray]; // Allocate array of integers

    //Serial.print("Resized arrays to: ");
    //Serial.println(sizeOfObjectArray);
  
  } else {
    Serial.print("Invalid new size: ");
    Serial.println(newSize);
  }
}

void updateCounterObjects(JsonArray groups) {
  int newSize = groups.size();
  resizeArrays(newSize);  // Resize the arrays to fit the new data

  // Loop through each item in the groups array
  for (int i = 0; i < newSize; i++) {
    // Get the group ID from the JSON
    int groupID = groups[i]["ID"];
    counterObjGroupIDs[i] = groupID;  // Save the ID in the global array

    // Calculate length and last digit of the group ID
    int length = String(groupID).length();
    int lastDigit = groupID % 10;

    // Create a Counter object using the length and last digit
    Counter* counter = new Counter(length, lastDigit);

    // Check if the Counter object is valid before assigning
    if (counter->isValid()) {
      counterObjs[i] = counter;  // Assign the pointer to the array
    } else {
      Serial.println("The counter could not be created as it was invalid.");
      delete counter;  // Free the memory if the object is invalid
    }
  }
}

void saveDataConfig(const DynamicJsonDocument& doc_new) {
  DynamicJsonDocument doc = loadJsonFromFile(); // get the current json
  DynamicJsonDocument old_saved_modes(512); // Adjust size as necessary
  old_saved_modes["savedModes"].set(doc["savedModes"].as<JsonObject>()); // must make a deep copy
  
  JsonArray old_groups = doc["configData"]["groups"].as<JsonArray>(); // this doesn't deep copy, they simply reference the parents memory
  JsonArrayConst new_groups = doc_new["configData"]["groups"].as<JsonArrayConst>();

  doc["resetFlag"] = false; // reset the flag
  doc["savedModes"] = doc_new["savedModes"]; // copy accross the modes
  JsonArray new_savedModes = doc["savedModes"].as<JsonArray>();

  // this for loop is to restore data if groups are unchanged between saves
  for (JsonObjectConst group : new_groups) { // for each group in the new groups
    int oldSavedModeID;
    bool keepChecking = false; // if we don't have the same group we can delete old data
    for (JsonObject oldGroup : old_groups) { // then for each old group
      if (oldGroup["ID"] == group["ID"]) { // if the group ID's match (so we still have the same group)
        oldSavedModeID = oldGroup["savedModeID"];
        keepChecking = true; // if we have a matching group we may need to restore data
        break;
      }
    }

    if (!keepChecking) {
      break; // move onto the next group
    }

    String old_name;
    JsonObject data;
    for (JsonObject oldSavedMode: old_saved_modes["savedModes"].as<JsonArray>()) {
      if (oldSavedMode["ID"] == oldSavedModeID) { // find the corresponding old saved mode
        old_name = oldSavedMode["name"].as<String>();
        data = oldSavedMode["data"].as<JsonObject>(); // save a copy of the data
        break;
      }
    }

    for (JsonObject savedMode : new_savedModes) { // then for each saved mode
      if (group["savedModeID"] == savedMode["ID"]) {  // once we have found the corresponding saved mode
        if (savedMode["name"].as<String>() == old_name) { // if the names match (so the same groups have the same modes)
          savedMode["data"] = data; // get old data and move it to the new doc
        }
        break;
      }
    }
  }

  doc["configData"] = doc_new["configData"]; // copy accross the new config (this will update old_groups too, thus must be done last)

  saveJsonToFile(doc); // modes remain unchanged

  setOverrideMode(doc["configData"]["override"].as<bool>());
  updateCounterObjects(doc["configData"]["groups"]);
}

uint8_t saveDataSavedModes(const DynamicJsonDocument& doc_new, bool clockAPICalling) {
  /*
    doc_new:
    .savedModeID -> the target saved mode

    .data -> the new JSON data to save (if called by Websocket)
    .state -> the new state (if called by Clock API)
  */

  DynamicJsonDocument doc = loadJsonFromFile(); // get the current json

  // Get the savedModeID and new data from doc_new
  int targetSavedModeID = doc_new["savedModeID"];

  bool modeFound = false;  // To track if we found the mode
  String modeName;

  // Search through the savedModes array for a matching savedModeID
  for (JsonObject mode : doc["savedModes"].as<JsonArray>()) {
    if (mode["ID"] == targetSavedModeID) {
      modeFound = true;
      modeName = mode["name"].as<String>();

      if (!clockAPICalling) { // if we are being called from the websocket to refresh saved data
        // Check if 'data' exists in the found mode
        if (mode.containsKey("data")) {
          // If 'data' exists, update it with the new data
          mode["data"] = doc_new["data"].as<JsonObjectConst>();

        } else {
          // If 'data' does not exist, create it and assign the new data
          mode.createNestedObject("data");
          mode["data"] = doc_new["data"].as<JsonObjectConst>();
        }
      
      } else { // if we are being called by the API endpoint to update state of the clock
        if (mode.containsKey("data")) {
          mode["data"]["state"] = doc_new["state"].as<String>();

        } else {
          // If 'data' does not exist throw the error back to the caller
          return 2;
        }
      }

      break; // Exit the loop once the mode is found and updated
    }
  }

  // If no matching mode was found, print an error
  if (!modeFound) {
    Serial.println("Error: Saved mode not found when trying to update");
    return 1;  // Exit the function if no mode was found
  
  } else {
    saveJsonToFile(doc); // everything else remains unchanged
        
    if (modeName.equals("Clock")) { // if we have just updated a clock, we need to get it started
      refreshClockStates();
    
    } else if (modeName.equals("Countdown")) { // if we have just updated a countdown, we need to get it started
      refreshCountdownStates();
    }

    return 0; // return success
  }
}

void saveDataReset() {
  DynamicJsonDocument doc = loadJsonFromFile(); // get the current json
  doc["resetFlag"] = true;
  saveJsonToFile(doc); // everything else remains unchanged
}

void displayData(const DynamicJsonDocument& doc, bool messageServer) {
  /*
    .groupID -> the group ID to display the content on
    .content -> the number or word
    .type -> if it is a word ('text') or number ('num')
    .align -> used if an alignment is specified (L, R, C)
    .mode -> name of the mode, only actually used if it is a special condition, e.g. Dice will make it do an animation before displaying
  */
  
  int groupID = doc["groupID"];
  bool found = false;
  for (int i = 0; i < sizeOfObjectArray; ++i) { // for each counter object
    if (counterObjGroupIDs[i] == groupID) { // if the groupIDs match

      String response = "Displayed '" + doc["content"].as<String>() + "' on groupID " + String(groupID);
      
      if (messageServer) {
        server.send(200, "text/plain", response);  // Send back a plain text response
      }
      
      found = true;
      

      // FOR SPECIAL EFFECTS ON SOME MODES
      String modeName = doc["mode"].as<String>();  // Get the mode name as a string
      if (modeName.equals("Dice")) {
        counterObjs[i]->rollEffect();
      }  // else if () {}... // for more special modes


      char alignment = doc["align"].as<String>().charAt(0); // get the alignment

      if (doc["type"].as<String>().equals("num")) {  // We need to show a number
        long number = doc["content"].as<long>();
        counterObjs[i]->displayNumber(number, alignment);
        
      } else if (doc["type"].as<String>().equals("text")) {  // Show a word
        String word = doc["content"].as<String>();
        counterObjs[i]->displayWord(word, alignment);  // Show the word
      }
      
      break; // stop looking
    }
  }

  if (!found && messageServer) {
    server.send(400, "text/plain", "Error: No matching group ID");  // Send an error response in plain text
  }
}

void loadData(int client, const DynamicJsonDocument& doc) {
  const char* contentType = doc["content"];  // Get the content as a string

  DynamicJsonDocument loadedDoc (1024);
  loadedDoc  = loadJsonFromFile(); // always read from the file

  if (doc.isNull()) {
    Serial.println("An error occurred when loading JSON from file");
    webSocket.sendTXT(client, "error"); // Tell the web that an error occurred
    return;
  
  } else { // no error
    if (strcmp(contentType, "All") == 0) { // We need to load all data
      String allJson; // Create a String to hold all JSON data
      serializeJson(loadedDoc, allJson); // Serialize the loadedDoc back to a JSON string
      webSocket.sendTXT(client, allJson); // Send the JSON string
      return;

    } else {
      Serial.print("Unknown load content type: ");
      Serial.println(contentType);
      return;
    }
  }
}

// To save the JSON to a file, the doc must be DynamicJsonDocument type
void saveJsonToFile(const DynamicJsonDocument& doc) {
  // Open a file for writing
  File file = LittleFS.open("/savedData.json", "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  // Serialize the JSON document to the file
  if (serializeJson(doc, file) == 0) {
    Serial.println("Failed to write to file");
  }

  // Close the file to save the changes
  file.close();
}

// used to load back the saved data (will return an empty doc if an error occours)
DynamicJsonDocument loadJsonFromFile() {
  // Open the file for reading
  File file = LittleFS.open("/savedData.json", "r");
  
  // Create a buffer to hold the loaded JSON document
  DynamicJsonDocument doc(1024);

  if (!file) {
    Serial.println("Failed to open file for reading");
    return doc;  // Return empty doc if the file cannot be opened
  }

  // Deserialize the JSON document from the file
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.print("Failed to parse file: ");
    Serial.println(error.c_str());
    return doc;  // Return empty doc if deserialization fails
  }

  // Close the file after reading
  file.close();

  return doc;  // Return the populated document
}

static String getContentType(String filename) {
  if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

static bool handleFileRead(String path) { // send the right file to the client (if it exists)
  ////Serial.println("handleFileRead: " + path);
  
  // Ignore URL parameters
  int paramIndex = path.indexOf('?');
  if (paramIndex >= 0) {
    path = path.substring(0, paramIndex); // Truncate the path to ignore parameters
  }

  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  
  if (LittleFS.exists(pathWithGz) || LittleFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (LittleFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed version
    File file = LittleFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    ////Serial.println(String("\tSent file: ") + path);
    return true;
  }
  
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}

void freeCounterObjMemory() {
  // Free the memory of the Counter objects first
  if (counterObjs != nullptr) {
    for (int i = 0; i < sizeOfObjectArray; i++) {
      delete counterObjs[i];  // Free each individual Counter object
    }
    delete[] counterObjs;  // Free the array of pointers
  }

  // Free the array of group IDs
  if (counterObjGroupIDs != nullptr) {
    delete[] counterObjGroupIDs;
  }

  // Reset the pointers to avoid dangling pointers
  counterObjs = nullptr;
  counterObjGroupIDs = nullptr;
  sizeOfObjectArray = 0;
}
