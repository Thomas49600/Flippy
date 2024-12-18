import { getUrlData } from './modes.js';
import { sendData, getSavedInfo } from './websocket.js';

const toggle = document.getElementById('formatCheckbox');
const img1 = document.getElementById('first-h-img');
const img2 = document.getElementById('second-h-img');

const urlData = getUrlData(); // this will also update the group ID text
const savedModeID = urlData.savedModeID;
const numDigits = String(urlData.groupID).length;
let modeData = {timezoneOffset: 0, format: 12, state: "off"};
let lastSavedModeData;

const onOffText = document.getElementById("onOffText");
const onColour = '#04AA6D';
const offColour = '#f44336'; // make sure to update HTML if you change this (as it is default red to start)

if (numDigits == 4) {
    document.getElementById("am-pm").textContent = ''; // clear the text
    document.getElementById("time-format-text").textContent = ''; // clear the text
    document.getElementById("HH-MM").textContent = 'Time format HH:MM'; // change the middle text

} else {
    document.getElementById('am-img').style.display = 'block';
    toggle.disabled = true;

    // Select the container where the changes will be made
    const container = document.querySelector('.page-container19');
    
    // Clear the existing content
    container.innerHTML = '';

    // Add the new content
    const newMessage = document.createElement('span');
    newMessage.textContent = 'If you wish to enable 24H time, please only use 4 digits';
    container.appendChild(newMessage);
}

setTimeout(() => { // wait for the connection
    getSavedInfo()
        .then((savedInfo) => {
            // Parse the JSON data
            const data = JSON.parse(savedInfo);

            // Store the formatted data in variables
            const savedModes = data.savedModes;
            const matchingMode = savedModes.find(mode => mode.ID == savedModeID);

            if (matchingMode) { // if there is a matching mode
                if (matchingMode.data) { // if it has a data key
                    console.log("Saved Data:", matchingMode.data);
                    modeData = structuredClone(matchingMode.data);; // load saved mode data

                } else {
                    console.log("No saved data available for this mode");
                    // maintain default mode data
                }

            } else {
                console.error("No matching saved mode");
            }
            
        })

        .catch((error) => {
            console.error('Error retrieving saved info:', error);
        }
    );

    setTimeout(() => { // wait for the connection
        // LOAD SAVED DATA
        if (modeData.format == 24) {
            img1.src = '2.jpg';
            img2.src = '4.jpg';
            toggle.checked = true;
        }

        document.getElementById('timezone-dropdown').value = modeData.timezoneOffset;
        
        if (modeData.state == "on") {
            onOffText.style.color = onColour;
            onOffText.textContent = 'on';

        } else {
            onOffText.style.color = offColour;
            onOffText.textContent = 'off';
        }

        // FINISHED LOADING

        lastSavedModeData = structuredClone(modeData);
    }, 400); // wait for the responce
}, 400);

toggle.addEventListener("input", () => {
    if (toggle.checked == true) {
        modeData.format = 24;
        img1.src = '2.jpg';
        img2.src = '4.jpg';

    } else {
        modeData.format = 12;
        img1.src = '1.jpg';
        img2.src = '2.jpg';
    }
});

let count = 0;
document.getElementById('startButton').addEventListener('click', function() {
    if (modeData.state == "off") { // if it is off
        count = 0;
        modeData.state = "on"; // mark it as on
        lastSavedModeData.state = "on";
        changeClockState("on"); // send message to esp
        displayBottomMessage('Turned on');

        onOffText.style.color = onColour;
        onOffText.textContent = 'on';
    
    } else {
        count += 1;

        if (count >= 2) {
            displayBottomMessage("Already On - Make sure you've saved both the configuration (main menu) and settings (this page)", true);
        } else {
            displayBottomMessage("Already On");
        }
    }
});

document.getElementById('saveButton').addEventListener('click', function() {
    showModalText();
});

document.getElementById('stopButton').addEventListener('click', function() {
    if (modeData.state == "on") { // if it is on
        count = 0;
        modeData.state = "off"; // mark it as off
        lastSavedModeData.state = "off";
        changeClockState("off"); // send message to esp
        displayBottomMessage('Turned off');

        onOffText.style.color = offColour;
        onOffText.textContent = 'off';

    } else {
        count += 1;

        if (count >= 2) {
            displayBottomMessage("Already off - Make sure you've saved both the configuration (main menu) and settings (this page)", true);
        } else {
            displayBottomMessage("Already Off");
        }
    }
});

// Function to close the modal with fade-out effect
function closeModalFunc() {
    const confirmModal = document.getElementById('confirmModal');
    confirmModal.classList.remove('show'); // Remove the show class to trigger fade-out

    // Wait for the fade-out transition to finish before hiding the modal
    setTimeout(() => {
        confirmModal.style.display = 'none'; // Hide modal completely
    }, 300); // Match this timeout to your CSS transition duration
}


// Hide the modal when "No" is clicked
document.getElementById('confirmNo').addEventListener('click', () => {
    closeModalFunc(); // Call the closeModal function
});

// Hide the modal when the close "X" is clicked
document.getElementById('closeModal').addEventListener('click', () => {
    closeModalFunc(); // Call the closeModal function
});

document.getElementById('timezone-dropdown').addEventListener("input", () => {
    modeData.timezoneOffset = document.getElementById('timezone-dropdown').value; // get the current chosen timezone
    // console.log(modeData.timezoneOffset);
});

// If "Yes" is clicked, show the settings message and hide the modal
document.getElementById('confirmYes').addEventListener('click', () => {
    closeModalFunc();

    console.log({savedModeID: urlData.savedModeID, data: modeData});
    let success = sendData("savedMode", {savedModeID: urlData.savedModeID, data: modeData});

    if (success == true) {
        displayBottomMessage('Settings have been saved!');
        lastSavedModeData = structuredClone(modeData);

    } else {
        displayBottomMessage('Failed to save settings (Try refreshing the page)');
    }
});

// Add event listener to warn the user before closing the page
window.addEventListener('beforeunload', function (event) {
    if (JSON.stringify(lastSavedModeData) !== JSON.stringify(modeData)) {
        // Custom message may not always be shown by all browsers
        const message = "You have unsaved changes. Are you sure you want to leave?";
        event.returnValue = message;  // Required for some browsers
        return message;  // For modern browsers
    }
});


let fadeOutTimeout; // Declare variable to store the fade-out timeout ID
let hideTimeout;    // Declare variable to store the hide timeout ID

function displayBottomMessage(text, extraTime=false) {
    const messageText = document.getElementById("messageText");
    messageText.textContent = text;

    // Show the settings message at the bottom
    messageContainer.style.display = 'block'; // Show the message container
    messageContainer.style.opacity = '0.95'; // Fade in

    // If the function is called again, clear the previous timeouts
    clearTimeout(fadeOutTimeout);  // Cancel the previous fade-out timeout
    clearTimeout(hideTimeout);     // Cancel the previous hide timeout

    let bonus = 0;
    if (extraTime) {
        bonus = 3000;
    }

    // Hide the message after 2 seconds
    fadeOutTimeout = setTimeout(() => {
        messageContainer.style.opacity = '0'; // Fade out
        hideTimeout = setTimeout(() => {
            messageContainer.style.display = 'none'; // Completely hide after fade-out
        }, 500); // Allow time for fade-out transition
    }, 2500+bonus);
}

// Function to update text content based on button pressed
function showModalText() {
    const modal = document.getElementById('confirmModal');

    modal.style.display = 'block'; // Set the display to flex before adding the class
    setTimeout(() => {
        modal.classList.add('show'); // Trigger the fade-in effect
    }, 0); // Execute immediately after setting display
}


function changeClockState(state) {
    const jsonData = {
        savedModeID: urlData.savedModeID,
        groupID: urlData.groupID,
        state: state
    }
    
    // Send the JSON data as a POST request to /esp-apis/clock
    fetch('/esp-apis/clock', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify(jsonData),
    })
    .then(response => {
        return response.text().then((text) => {
            // Log the response body regardless of state code
            console.log('Response body:', text);
            
            setTimeout(() => {
                if (text == "No matching saved mode was found") {
                    displayBottomMessage("An error occoured: You are likley on an outdated sub-page. Please resave the configuration (main menu) and follow the settings button provided", true);
                    modeData.state = "off";
                    lastSavedModeData.state = "off";
                    onOffText.style.color = offColour;
                    onOffText.textContent = 'off';

                } else if (text == "The matching saved mode had no data already stored") {
                    displayBottomMessage("An error occoured: Make sure you save the settings first", true);
                    modeData.state = "off";
                    lastSavedModeData.state = "off";
                    onOffText.style.color = offColour;
                    onOffText.textContent = 'off';
                }
            }, 50); // a slight delay to replace the existing popup

            if (!response.ok) {
                throw new Error('Network response was not ok');
            }
            return text;
        });
    })
    .then(data => {
        console.log('Success:', data);
    })
    .catch((error) => {
        console.error('Error:', error);
    });
}