// WebSocket connection (defined once and shared across imports)
const connection = new WebSocket('ws://' + location.hostname + ':81/', ['arduino']);
const homePage = 'http://flippy.local' // https://localhost:4443
export { homePage };


connection.onopen = function () {
  console.log('Connected to websocket ' + new Date()); 
  //connection.send('Connected to websocket ' + new Date()); 
};
connection.onerror = function (error) {
  console.error('WebSocket Error ', error);
};
connection.onclose = function () {
  console.log('WebSocket connection closed');
};


/*
The type of data is either the config data, or an updated saved mode. Data must be a JSON document

The config data should be formatted as so:
.resetFlag -> false (this should always be false to signify we have new saved data)
.configData
  .override -> true or false
  .config -> the number of digits with #'s representing gaps
  .groups -> an array of the following two items [(ID, savedModeID), (..., ...) ...]
    .ID -> the group's ID, such as 432 or 21 or 4 (aka the PK)
    .savedModeID -> the ID of the saved mode that corresponds to this counters mode (aka a FK)
  
.savedModes -> an array of the following [(ID, name), (..., ...) ...]
  .ID -> the unique ID of the saved mode (aka the PK), generated when SET is pressed
  .name -> the name of the mode displayed in the dropdown options menu, default is 'None'

The saved mode data will vary but should contain
.savedModeID -> the ID of the saved mode (aka the PK)
.data -> an object that will have different attributes depending on the mode (e.g. [.timezone -> "0", .format -> "12H", .status -> "on"] for a clock)

The reset flag data should always be true (signifying we want to set the flag)
.resetFlag -> true

Show number and show word should be as follows: (NOW MOVED TO AN API ENDPOINT /display)
  .groupID -> the group ID to display the content on
  .content -> the number or word
  .type -> if it is a word ('text') or number ('num')
  .align -> used if an alignment is specified (L, R, C)
  .mode -> name of the mode, only actually used if it is a special condition, e.g. Dice will make it do an animation before displaying
*/

// Calling ID's are as follows:
const savingIDs = {
  configData: 0,
  savedMode: 1,
  resetFlag: 2,
  requestData: 3
};

export function sendData(type, data) {
  let success = true;
  var typeID = savingIDs[type]; // Get the typeID based on the provided type

  if (typeID === undefined) {
    console.error('Invalid type provided:', type);
    return; // Exit the function if the type is invalid
  }

  // Append the "type" field to the data object
  data.type = typeID; // Add the type field to the data object

  // Serialize the updated data object to JSON string format
  var formattedData = JSON.stringify(data); // Convert the data object to a JSON string

  // Check if the serialized JSON string exceeds the size limit (1024 bytes in this case)
  const maxSize = 980; // Define the size limit in bytes
  if (formattedData.length > maxSize) {
    console.error(`Data size exceeds the limit of ${maxSize} bytes. Data not sent.`);
    return false; // Indicate failure to send data
  }

  if (connection.readyState !== WebSocket.OPEN) {
    console.error('WebSocket is not open. Current state:', connection.readyState);
    return false; // Indicate failure to send data
  }

  // Create a temporary error handler for this specific send operation
  const onError = (event) => {
    console.error('WebSocket error occurred while sending data:', event);
    success = false;
  };

  // Attach the error listener to the WebSocket connection
  connection.addEventListener('error', onError);

  connection.send(formattedData); // Send the JSON string over WebSocket
  console.log('Data sent:', formattedData); // Log the data sent

  setTimeout(() => {
    connection.removeEventListener('error', onError);   // Remove the temporary error listener after a timeout of 2 seconds
  }, 2000);

  return success;
}


/*
will recieve the following info as a JSON document
.resetFlag -> signifying if we should use the saved data or not (true: reset back to default, false: load saved data)

.configData (updated on the config menu via "set")
  .override -> true or false
  .config -> the number of digits with #'s representing gaps
  .groups -> an array of the following two items [(ID, savedModeID), (..., ...) ...]
    .ID -> the group's ID, such as 432 or 21 or 4 (aka the PK)
    .savedModeID -> the ID of the saved mode that corresponds to this counters mode (aka a FK)
  
.savedModes (updated on each counters individual webpage) -> an array of the following [(ID, name, data), (..., ..., ...) ...]
  .ID -> the unique ID of the saved mode (aka the PK), generated when SET is pressed
  .name -> the name of the mode displayed in the dropdown options menu, default is 'None'
  .data -> an object that will have different attributes depending on the mode (e.g. [.timezone -> "0", .format -> "12H", .status -> "on"] for a clock)

.modes (hardcoded in the ESP8266's code) -> an array of the following [(name, constraints), (..., ...) ...]
  .name -> the name of the mode, this must be unique (aka the PK)
  .logic -> either AND or OR, and will mean all constraints must be true for it to be valid, or means only one constraint needs to be true, for modes with one constraint default to OR
  .constraints -> an array of the following two items below defining the size constraints of the counter for the mode to be a viable option
    .operator -> either > or < or =
    .value -> a number between (and inclusive of) 0 ... 8
    
    // constraints might include (for a clock, that must have either 4, or 6 digits for either hh:mm or hh:mm:ss)
    [(=, 4), (=, 6)] i.e. exactly 4 OR 6 digits

    // or for a counter which can have any number of digits
    [(>, 0)] i.e. any number of digits
*/

export function getSavedInfo() {
  return new Promise((resolve, reject) => {
    // Set a timeout to reject the promise after 10 seconds (10000 milliseconds)
    const timeout = setTimeout(() => {
        reject(new Error('Timeout: No message received after 10 seconds'));
        cleanup(); // Call the cleanup function on timeout
    }, 10000);

    // Function to clean up the event listeners
    function cleanup() {
        clearTimeout(timeout); // Clear the timeout (if still active)
        connection.removeEventListener('message', handleMessage); // Remove message listener
        connection.removeEventListener('error', handleError); // Remove error listener
    }

    // Event listener for message received
    function handleMessage(event) {
        console.log('Saved Info Received: ', event.data); // Print the message to console
        
        if (event.data == "error") { // if we got sent an error
            reject(new Error("An error occurred ESP-side when loading data")); // Reject the promise with an error
        
        } else {
            resolve(event.data); // Resolve the promise with the message (JSON)
        }

        cleanup(); // Call the cleanup function
    }

    // Event listener for errors
    function handleError(err) {
        console.error('WebSocket error:', err); // Log the error
        reject(err); // Reject the promise with the error
        cleanup(); // Call the cleanup function
    }

    // Add the event listeners
    connection.addEventListener('message', handleMessage);
    connection.addEventListener('error', handleError);

    // Attempt to request data, and if it fails, immediately reject the promise
    const success = sendData('requestData', { content: 'All' });

    if (!success) {
        reject(new Error('Failed to request data')); // Immediately reject the promise
        cleanup(); // Clean up the event listeners and timeout
    }
  });
  
  /*  
  return new Promise((resolve, reject) => {
      // Simulating a received JSON string over WebSocket
      const jsonString = JSON.stringify({
          "resetFlag": false,
          "configData": {
              "config": "1",
              "override": false,
              "groups": [
                  { "ID": 1, "savedModeID": 1 }
              ]
          },
          "savedModes": [
              { "ID": 1, "name": "Counter", "data": { "currentValue": 4, "maxValue": 9 } }
          ],
          "modes": [
              { "name": "Clock", "logic": "AND", "constraints": [{ "operator": ">", "value": 3 }, { "operator": "<", "value": 8 }] },
              { "name": "Counter", "logic": "OR", "constraints": [{ "operator": ">", "value": 0 }] },
              { "name": "Dice", "logic": "OR", "constraints": [{ "operator": ">", "value": 0 }] }
          ]
      });

      // Resolve with the JSON string
      resolve(jsonString);
  });
  */
}




/* Example loading saved data - JSON
{
  "resetFlag": true,
  "configData": {
    "config": "5#4321",
    "groups": [
      {
        "ID": 5,
        "savedModeID": 1
      },
      {
        "ID": 4321,
        "savedModeID": 2
      }
    ]
  },
  "savedModes": [
    {
      "ID": 1,
      "name": "Counter",
      "data": {
        "currentValue": 4,
        "maxValue": 9
      }
    },
    {
      "ID": 2,
      "name": "Clock",
      "data": {
        "timezone": "GMT+8",
        "setup": "hh:mm"
      }
    }
  ],
  "modes": [
    {
      "name": "Clock",
      "constraints": [
        {
          "operator": "=",
          "value": 4
        },
        {
          "operator": "=",
          "value": 6
        }
      ]
    },
    {
      "name": "Counter",
      "constraints": [
        {
          "operator": ">",
          "value": 0
        }
      ]
    },
    {
      "name": "Dice",
      "constraints": [
        {
          "operator": ">",
          "value": 0
        }
      ]
    }
  ]
}
*/