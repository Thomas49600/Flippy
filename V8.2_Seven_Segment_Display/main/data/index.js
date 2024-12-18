import { sendData, getSavedInfo, homePage } from './websocket.js';
let formattedData;

const addButton = document.getElementById('addButton');
const removeButton = document.getElementById('removeButton');
const setButton = document.getElementById('setButton');
const helpButton = document.getElementById('helpButton');
const resetButton = document.getElementById('resetButton');
const imageContainer = document.getElementById('imageContainer');
const messageContainer = document.getElementById('messageContainer');
const closeModal = document.getElementById('closeModal');
const confirmYes = document.getElementById('confirmYes');
const confirmNo = document.getElementById('confirmNo');
const loadingScreen = document.getElementById('loadingScreen');
const overrideCheckBox = document.getElementById('overrideCheckBox');
let possibleOptions = []; // Declare the variable globally


function updateSettingButtonsURLs() {
    // Check if formattedData is defined and has corresponding entries
    if (formattedData && formattedData.groups) {
        const dropdownButtons = document.querySelectorAll('.dropdown-button');  // Get all the dynamically generated buttons with the class 'dropdown-button'

        // Loop through each button
        dropdownButtons.forEach(button => {
            const groupID = button.getAttribute('groupID');
            
            if (groupID == null) {
                return; // skip
            }

            const groupIDNums = groupID.replace('group-', '');
            const matchingGroup = formattedData.groups.find(g => g.ID == groupIDNums);  // Find the group in formattedData.groups with the same ID
            let modeName;
            let savedModeID;

            if (matchingGroup) {
                savedModeID = matchingGroup.savedModeID; // Get the savedModeID from the matching group
                const savedMode = formattedData.savedModes.find(sm => sm.ID == savedModeID); // Find the corresponding saved mode name using savedModeID
                modeName = savedMode.name;
            }

            if (modeName == 'None') { // just display ID and not a button
                button.classList.add('noMode');
                button.textContent = `Counter ID: ${groupIDNums}`;

            } else { // an actual mode
                button.textContent = 'Settings';
                button.classList.remove('noMode');

                const dataToStore = {
                    groupID: groupIDNums,
                    savedModeID: savedModeID,
                    modeName: modeName
                };

                // Convert the object to a JSON string and Base64 encode it
                const jsonString = JSON.stringify(dataToStore);
                const encodedData = btoa(jsonString); // btoa() encodes the string to Base64

                button.onclick = function() {
                    // Open the new page with the encoded data as a parameter 'p'
                    window.open(`${homePage}/${modeName}.html?p=${encodedData}`, '_blank');
                };
            }
        });

    } else {
        console.error('formattedData is not defined at all/correctly')
    }
}

function updateSettingButtons() {
    const currentConfig = getGroupings(); // Get the current configuration string (e.g., '54#321')

    // Exit if the current config does not match the saved one or if formattedData is undefined
    if (!formattedData || formattedData.config !== currentConfig) {
        return; 
    }

    const currentGroups = currentConfig.split('#'); // Split it into an array of groups

    // Loop through each current group using a standard for loop
    for (let index = 0; index < currentGroups.length; index++) {
        const group = currentGroups[index]; // Get the current group
        const groupId = `group-${group}`; // Construct the group ID
        const currentName = selectedValues.get(groupId); // Get the current name from selectedValues

        // Check if formattedData is defined and has corresponding entries
        if (formattedData && formattedData.groups) {
            // Find the group in formattedData.groups with the same ID
            const matchingGroup = formattedData.groups.find(g => g.ID == group);
            
            if (matchingGroup) {
                const savedModeID = matchingGroup.savedModeID; // Get the savedModeID from the matching group

                // Find the corresponding saved mode name using savedModeID
                const savedMode = formattedData.savedModes.find(sm => sm.ID == savedModeID);
                
                // Check if the saved mode name matches the current name
                if (savedMode && savedMode.name !== currentName) {
                    return; // Exit the function if the names do not match
                }
            }
        }
    }
    
    // If all iterations complete without returning, show the setting buttons
    showSettingButtons();
}


function showSettingButtons() {
    // Get all the dynamically generated buttons with the class 'dropdown-button'
    const dropdownButtons = document.querySelectorAll('.dropdown-button');

    // Loop through each button and add the 'show' class
    dropdownButtons.forEach(button => {
        button.classList.add('show');
    });

    updateSettingButtonsURLs();
}

function hideSettingButtons() {
    // Get all the dynamically generated buttons with the class 'dropdown-button'
    const dropdownButtons = document.querySelectorAll('.dropdown-button');

    // Loop through each button and add the 'show' class
    dropdownButtons.forEach(button => {
        button.classList.remove('show');
    });
}

// Show welcome screen
function showWelcomeScreen() {
    const welcomeScreen = document.getElementById('welcomeScreen');
    document.getElementById('welcomeScreen').classList.add('force-hover');
    // Ensure element is visible before applying fade-in effect
    welcomeScreen.style.display = 'flex'; // Set the display to flex before adding the class
    setTimeout(() => {
        welcomeScreen.classList.add('show'); // Trigger the fade-in effect
    }, 10); // Slight delay to allow transition effect to trigger
}

// Hide welcome screen
function hideWelcomeScreen() {
    const welcomeScreen = document.getElementById('welcomeScreen');
    welcomeScreen.classList.remove('show'); // fade out
    document.getElementById('welcomeScreen').classList.remove('force-hover'); // Remove force-hover class
    setTimeout(() => {
        welcomeScreen.style.display = 'none'; // Actually remove it
    }, 350);
}


// Add event listener to continue button
document.getElementById('continueButton').addEventListener('click', hideWelcomeScreen);

// Show api screen
function showApiHelpScreen() {
    const ApiHelpScreen = document.getElementById('ApiHelpScreen');
    // Ensure element is visible before applying fade-in effect
    ApiHelpScreen.style.display = 'flex'; // Set the display to flex before adding the class
    setTimeout(() => {
        ApiHelpScreen.classList.add('show'); // Trigger the fade-in effect
    }, 10); // Slight delay to allow transition effect to trigger
}

// Hide api screen
function hideApiHelpScreen() {
    const ApiHelpScreen = document.getElementById('ApiHelpScreen');
    ApiHelpScreen.classList.remove('show'); // fade out
    setTimeout(() => {
        ApiHelpScreen.style.display = 'none'; // Actually remove it
    }, 350);
}

// Add event listener to continue button
document.getElementById('closeApiHelp').addEventListener('click', hideApiHelpScreen);
document.getElementById('apiHelpButton').addEventListener('click', showApiHelpScreen);

let dotInterval; // Variable to hold the interval ID

// Function to show the loading screen
function showLoadingScreen() {
    let dotCount = 0; // Counter for dots

    const loadingDots = document.getElementById('loadingDots'); // Ensure loadingDots is defined
    
    loadingScreen.style.display = 'flex'; // Set the display to flex before adding the class
    setTimeout(() => {
        loadingScreen.classList.add('show'); // Trigger the fade-in effect
    }, 10); // Slight delay to allow transition effect to trigger

    // Function to update dots
    function updateDots() {
        if (dotCount < 3) {
            dotCount++; // Increment dotCount up to 3
            const dot = document.createElement('span'); // Create a new span for the dot
            dot.className = 'loading-dot fade-in'; // Apply the fade-in class
            dot.textContent = '.'; // Set the dot character
            loadingDots.appendChild(dot); // Append the dot to the loadingDots container
        } else {
            // After three dots are shown, fade them out
            const dots = document.querySelectorAll('.loading-dot');
            dots.forEach(dot => {
                dot.classList.remove('fade-in'); // Remove fade-in class if present
                dot.classList.add('fade-out'); // Add fade-out class
            });

            // Reset after fading out
            setTimeout(() => {
                loadingDots.innerHTML = ''; // Clear all dots
                dotCount = 0; // Reset the dot count
            }, 300); // Match the duration of the fade-out animation
        }
    }

    // Start the dot update interval and store the interval ID
    dotInterval = setInterval(updateDots, 450);
}

// Function to hide the loading screen
function hideLoadingScreen() {
    loadingScreen.classList.remove('show'); // fade out
    setTimeout(() => {
        loadingScreen.style.display = 'none'; // Actually remove it
        // Clear the interval if it exists
        if (dotInterval) {
            clearInterval(dotInterval); // Stop updating the dots
            dotInterval = null; // Reset the interval ID
        }
    }, 350);
}


// Store selected values for each dropdown
const selectedValues = new Map();
const dropdownContainer = document.createElement('div'); // Container for dropdowns
dropdownContainer.className = 'dropdown-container';
document.body.appendChild(dropdownContainer); // Append to body or main container

let popupMode = "set"; // it is "set" as default

let imageCount = 1; // Start with 1 image
let firstTime = true;
const maxImages = 8;

function updateTipText() {
    // Select the elements with the 'button-label' class (lines of text)
    const buttonLabels = document.querySelectorAll('.button-label');

    // Check if the variable 'imageCount' is greater than 1
    if (imageCount > 1) {
        // Add the 'visable' class to each line of text
        buttonLabels.forEach(label => label.classList.add('visable'));
    } else {
        // Remove the 'visable' class if imageCount is equal to 1
        buttonLabels.forEach(label => label.classList.remove('visable'));
    }
}

// Array to hold the URLs of the images to preload
const preloadImages = [];

// Preload images
function preloadImage(url) {
    const img = new Image();
    img.src = url;
    img.loading = 'eager';
}

function createImage() {
    const newImageWrapper = document.createElement('div');
    newImageWrapper.className = 'image-wrapper';
    newImageWrapper.setAttribute('data-index', imageCount); // Assign a data-index to the image

    const triangleButton = document.createElement('button');
    triangleButton.className = 'triangle-button';
    triangleButton.innerHTML = '▼'; // Downward triangle
    triangleButton.dataset.action = 'increase'; // Set to increase space
    triangleButton.setAttribute('data-index', imageCount); // Assign a data-index to the button
    
    triangleButton.addEventListener('click', () => {
        newImageWrapper.classList.toggle('active');
        triangleButton.classList.toggle('extraSpace');

        dropdownContainer.classList.add('hidden'); // Fade out
        setTimeout(() => {
            createDropdowns();
            dropdownContainer.classList.remove('hidden'); // Fade in
        }, 300); // Delay for transition
    });

    const newImage = document.createElement('img');
    newImage.src = preloadImages[imageCount - 1]; // Use preloaded image
    newImage.alt = `Image ${imageCount}`;
    newImage.className = 'image';
    newImage.loading = 'eager';
    newImage.style.display = 'block'; // Ensure it's visible

    newImageWrapper.appendChild(triangleButton);
    newImageWrapper.appendChild(newImage);

    imageContainer.insertBefore(newImageWrapper, imageContainer.firstChild);
    setTimeout(() => {
        newImageWrapper.classList.add('visable'); // Fade in
    }, 10); // Delay for transition

    // Call to create dropdowns
    if (firstTime) {
        createDropdowns();
        firstTime = false;
    } else {
        dropdownContainer.classList.add('hidden'); // Fade out
        setTimeout(() => {
            createDropdowns();
            dropdownContainer.classList.remove('hidden'); // Fade in
        }, 300); // Delay for transition
    }
}


// After adding an image
addButton.addEventListener('click', () => {
    if (imageCount < maxImages) {
        imageCount++; // Increment image count before creating the new image
        createImage(); // will call createDropdowns();
        updateButtonVisibility(); // Ensure the buttons are shown/hidden correctly
        updateTipText();
    }
});

removeButton.addEventListener('click', () => {
    if (imageCount > 1) {
        const imageWrappers = imageContainer.querySelectorAll('.image-wrapper');
        const lastImageWrapper = imageWrappers[0];
        const lastButton = lastImageWrapper.querySelector('.triangle-button');

        // Remove the visibility (fade-out effect)
        lastImageWrapper.classList.remove('visable');
        lastButton.classList.remove('visable');

        // Remove the image from DOM after fade-out transition ends (0.5s delay)
        setTimeout(() => {
            imageContainer.removeChild(lastImageWrapper); // Remove the leftmost image
            imageCount--;

            // Update button visibility immediately after removing the image
            updateButtonVisibility(); // Update button visibility
            updateTipText();
        }, 200); // Match the CSS transition duration

        dropdownContainer.classList.add('hidden'); // Add hidden class to fade out
        setTimeout(() => {
            createDropdowns();
            dropdownContainer.classList.remove('hidden'); // Remove hidden class to fade in
        }, 450); // Delay by 100 milliseconds
    }
});


function updateButtonVisibility() {
    const imageWrappers = imageContainer.querySelectorAll('.image-wrapper');
    imageWrappers.forEach((wrapper, index) => {
        const button = wrapper.querySelector('.triangle-button');
        if (index === 0) { // Hide the first button (which is the latest added)
            button.classList.remove('visible'); // Remove visible class
            button.classList.remove('extraSpace'); // Remove visible class
            wrapper.classList.remove('active'); // Reset to default state (no gap)
        } else {
            button.classList.add('visible'); // Add visible class to other buttons
        }
    });
}


// Function to update text content based on button pressed
function showModalText(buttonPressed) {
    const confirmMessageText = document.getElementById("confirmMessageText");
    const confirmYes = document.getElementById("confirmYes");
    const confirmNo = document.getElementById("confirmNo");

    const modalContent = document.querySelector('.modal-content'); // Select modal content
    const modal = document.getElementById('confirmModal');

    // Check which button was pressed and set text accordingly
    if (buttonPressed === "set") {
        popupMode = "set";
        confirmMessageText.textContent = "Are you sure you want to save these settings? This will override any existing settings";
        confirmYes.textContent = "Save";
        confirmNo.textContent = "Cancel";
        modalContent.style.width = '450px'; // Set desired width for set button

    } else if (buttonPressed === "reset") {
        popupMode = "reset";
        confirmMessageText.textContent = "Resetting back to the default settings cannot be undone.";
        confirmYes.textContent = "Reset";
        confirmNo.textContent = "Cancel";
        modalContent.style.width = '500px'; // Set desired width for reset button
    }

    modal.style.display = 'block'; // Set the display to flex before adding the class
    setTimeout(() => {
        modal.classList.add('show'); // Trigger the fade-in effect
    }, 0); // Execute immediately after setting display
}


// Show the custom confirmation modal when the SET button is clicked
setButton.addEventListener('click', () => {
    showModalText("set");
});

resetButton.addEventListener('click', () => {
    showModalText("reset");    
});


helpButton.addEventListener('click', () => {
    showWelcomeScreen();
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
confirmNo.addEventListener('click', () => {
    closeModalFunc(); // Call the closeModal function
});

// Hide the modal when the close "X" is clicked
closeModal.addEventListener('click', () => {
    closeModalFunc(); // Call the closeModal function
});


function saveConfigData() {
    /*
        The config data should be formatted as so:
        .configData
            .config -> the number of digits with #'s representing gaps
            .groups -> an array of the following two items [(ID, savedModeID), (..., ...) ...]
                .ID -> the group's ID, such as 432 or 21 or 4 (aka the PK)
                .savedModeID -> the ID of the saved mode that corresponds to this counters mode (aka a FK)
        
        .savedModes -> an array of the following [(ID, name), (..., ...) ...]
            .ID -> the unique ID of the saved mode (aka the PK), generated when SET is pressed
            .name -> the name of the mode displayed in the dropdown options menu, default is 'None'
    */
    let jsonData = {configData: {}, savedModes: {}};
    
    const config = getGroupings(); // get the layout; e.g. 54#321
    jsonData.configData.config = config;
    
    const groupings = config.split('#'); // split the layout into an array; e.g. [54, 321]
    let groups = [];
    for (let i = 0; i < groupings.length; i++) {
        const currentGroup = {ID: groupings[i], savedModeID: i+1}; // the reason savedMode has a unique ID and not just groupID, is for the future potential of 'linking' where two counters are linked to one savedMode (so they replicate eachother)
        groups.push(currentGroup);
    }
    jsonData.configData.groups = groups;

    let savedModes = [];
    for (const group of groups) {
        const name = selectedValues.get(`group-${group.ID}`); // get the group ID of the group and thus find the name of the current mode
        const currentMode = {ID: group.savedModeID, name: name};
        savedModes.push(currentMode);
    }
    jsonData.savedModes = savedModes;


    if (overrideCheckBox.checked) {
        jsonData.configData.override = true;
    } else {
        jsonData.configData.override = false;
    }

    console.log(jsonData);
    let success = sendData("configData", jsonData); // send the data to the ESP8266

    if (success) { // if success, we need to update the stored saved data
        formattedData.resetFlag = false;
        formattedData.config = jsonData.configData.config;
        formattedData.groups = jsonData.configData.groups;
        formattedData.override = jsonData.configData.override;
        formattedData.savedModes = jsonData.savedModes;
        //updateSettingButtonsURLs();
    }

    return success;
}


function displayBottomMessage(text) {
    const messageText = document.getElementById("messageText");
    messageText.textContent = text;

    // Show the settings message at the bottom
    messageContainer.style.display = 'block'; // Show the message container
    messageContainer.style.opacity = '0.95'; // Fade in

    // Hide the message after 2 seconds
    setTimeout(() => {
        messageContainer.style.opacity = '0'; // Fade out
        setTimeout(() => {
            messageContainer.style.display = 'none'; // Completely hide after fade-out
        }, 500); // Allow time for fade-out transition
    }, 2500); 
}

// If "Yes" is clicked, show the settings message and hide the modal
confirmYes.addEventListener('click', () => {
    closeModalFunc();

    if (popupMode === "set") {
        let success = saveConfigData(); // save the data

        if (success == true) {
            displayBottomMessage('Settings have been saved!');
            showSettingButtons(); // show buttons if we successfully saved
        } else {
            displayBottomMessage('Failed to save settings (try refreshing the page)');
        }
        
    
    } else if (popupMode === "reset") {
        let success = sendData("resetFlag", {resetFlag: true});
        formattedData.resetFlag = true;

        if (success == true) {
            displayBottomMessage('Resetting...'); 
            setTimeout(() => {
                location.reload();
            }, 1800); // wait two sec for data transfer 
        
        } else { // failed to send data
            displayBottomMessage('Failed to reset settings');   
        }
    }
});



function getGroupings() {
    const imageWrappers = imageContainer.querySelectorAll('.image-wrapper');
    let groupings = ''; // To hold the final grouping string

    imageWrappers.forEach((wrapper, index) => {
        const triangleButton = wrapper.querySelector('.triangle-button'); // Correct class name
        
        // Check if the triangle button has the .extraSpace class
        if (triangleButton && triangleButton.classList.contains('extraSpace')) {
            // If it has extra space, it indicates a gap
            groupings += '#';
        }
        groupings += String(imageCount - index); // always add the number regardless
    });

    //console.log(groupings); // Print the grouping string
    return groupings; // Return the grouping string
}


// Function to create dropdowns for grouped images based on groupings
function createDropdowns(resize=false) {
    if (!resize) { // if we are not resizing
        hideSettingButtons(); // hide the buttons, as if we are creating dropdowns this means we have clicked some button to change the layout
    }
    
    // Store the current state of the buttons before clearing the container
    const buttonsState = {};
    const buttons = dropdownContainer.querySelectorAll('.dropdown-button');
    buttons.forEach(button => {
        const groupID = button.getAttribute('groupID');
        buttonsState[groupID] = button.classList.contains('show'); // Save whether each button has 'show' class
    });
    
    // Remove existing dropdowns
    dropdownContainer.innerHTML = '';

    // Get groupings string
    const groupingsString = getGroupings();

    // Split the string by gaps
    const groupings = groupingsString.split('#');

    // Loop through each grouping to create dropdowns
    groupings.forEach((group, groupIndex) => {
        const currentGroup = [];

        // Convert string to array of numbers
        const indices = group.split('').map(Number);

        // Retrieve image wrappers for each index
        indices.forEach(index => {
            const imageWrapper = imageContainer.querySelector(`.image-wrapper:nth-child(${imageCount - index + 1})`);
            if (imageWrapper) {
                currentGroup.push(imageWrapper);
            }
        });

        // Generate a unique ID for the current group
        const groupId = `group-${indices.join('')}`;
        //console.log(`Creating dropdown for ${groupId}`); // For debugging

        // Create a dropdown for the current group
        if (currentGroup.length > 0) {
            createDropdownForGroup(currentGroup, groupId, resize); // Pass the unique groupId

            // Reapply the 'show' class if the button was shown before
            const newButton = dropdownContainer.querySelector(`button[groupID="${groupId}"]`);
            if (buttonsState[groupId]) {
                newButton.classList.add('show');
            }
        }
    });

    setTimeout(function() {
        updateSettingButtons();
    }, 10);
    
}


function getPossibleOptions(numberOfDigits, modes) {
    let options = ['None'];

    for (const mode of modes) { // for each mode
        let modeIsValid = 0; // default to false
        if (mode.logic == "OR" || mode.logic == "AND") { // if the logic is valid
            let logic = mode.logic;

            for (const constraint of mode.constraints) {
                switch (constraint.operator) {
                    case "=":
                        if (numberOfDigits == constraint.value && modeIsValid != 3) {
                            modeIsValid = 1; // set to true
                        } else if (logic == "AND") { // if logic is OR, leave modeIsValid unchanged (this way once it is true, it will always be true)
                            modeIsValid = 3; // however if logic is AND and the constraint is violated, we want to set it to 3, indicating it will always be invalid
                        }
                        break;
                    
                    case ">":
                        if (numberOfDigits > constraint.value && modeIsValid != 3) {
                            modeIsValid = 1;
                        } else if (logic == "AND") {
                            modeIsValid = 3;
                        }
                        break;
    
                    case "<":
                        if (numberOfDigits < constraint.value && modeIsValid != 3) {
                            modeIsValid = 1;
                        } else if (logic == "AND") {
                            modeIsValid = 3;
                        }
                        break;
    
                    default:
                        console.log(`Unrecognised opperator ${constraint.operator}`);
                        modeIsValid = 0; // make sure it is false
                        break;
                }
            }

        } // if the logic is not valid, it skips constraint checks and leaves modeIsValid to false
    
    
        if (modeIsValid == 1) { // if the mode is valid, add it to the end of the array
            options.push(mode.name)
        }
    }

    return options
}



// Function to create a dropdown under a group of images
function createDropdownForGroup(group, groupId, resize) {  
    const dropdown = document.createElement('select');
    dropdown.className = 'image-dropdown';

    // Position dropdown below the images
    const firstImage = group[0];
    const lastImage = group[group.length - 1];

    // Calculate dropdown width based on the group
    const groupWidth = lastImage.offsetLeft + lastImage.offsetWidth - firstImage.offsetLeft;
    dropdown.style.width = `${groupWidth}px`;
    dropdown.style.left = `${firstImage.offsetLeft}px`; // Align left with the first image in group

    // Add dropdown options (example options)
    const numberOfDigits = (groupId.replace('group-', '')).length;
    const options = possibleOptions[numberOfDigits-1];
    options.forEach(optionText => {
        const option = document.createElement('option');
        option.textContent = optionText;
        dropdown.appendChild(option);
    });

    // Check if a value for this groupId exists in selectedValues
    if (selectedValues.has(groupId)) {
        dropdown.value = selectedValues.get(groupId); // Set to last chosen value
    } else {
        dropdown.value = options[0]; // Set to default if no value is stored
        selectedValues.set(groupId, dropdown.value); // Store the selected value
    }

    // Event listener to update selected value on change
    dropdown.addEventListener('change', () => {
        selectedValues.set(groupId, dropdown.value); // Store the selected value using groupId
        if (!resize) { // Only hide the buttons if not resizing
            hideSettingButtons();
            updateSettingButtons();
        }
    });

    dropdown.setAttribute('groupID', groupId); // Assign an ID to the dropdown
    //console.log(groupId);

    // Create a button to replace the label
    const button = document.createElement('button');
    button.className = 'dropdown-button';
    button.setAttribute('groupID', groupId); // Assign an ID to the button

    // Position the button below the dropdown
    button.style.position = 'absolute';
    button.style.top = `${dropdown.offsetTop + dropdown.offsetHeight + 45}px`; // Slightly below the dropdown
    button.style.left = dropdown.style.left;
    button.style.width = dropdown.style.width; // Make the button the same width as the dropdown

    // Append dropdown and button to the container
    dropdownContainer.appendChild(dropdown);
    dropdownContainer.appendChild(button);
}


window.onresize = function() {
    createDropdowns(true); // Call the function when the window is resized
};


// Add event listener to warn the user before closing the page
window.addEventListener('beforeunload', function (event) {
    const firstDropdownButton = document.querySelector('.dropdown-button');
    let shouldWarnUser = !firstDropdownButton.classList.contains('show'); // if the blue buttons are hidden, we have an unsaved config

    if (shouldWarnUser) {
        // Custom message may not always be shown by all browsers
        const message = "You have unsaved changes. Are you sure you want to leave?";
        event.returnValue = message;  // Required for some browsers
        return message;  // For modern browsers
    }
});


function loadData(config, groups, savedModes, override) {
    if (override == false) { // it is defaulted to true
        overrideCheckBox.checked = false;
    }

    let cleanConfig = config.replace(/#/g, ''); // Remove all hashtags

    // Loop through the clean string
    for (let i = 0; i < cleanConfig.length-1; i++) {
        if (imageCount < maxImages) {
            imageCount++; // Increment image count before creating the new image
            createImage(); // will also call createDropdowns();
        }
    }

    // images and dropdowns are now displayed //

    updateButtonVisibility(); // Ensure the buttons are shown/hidden correctly
    updateTipText();

    // text and buttons are now displayed //

    for (let i = 0; i < config.length; i++) { // for each image
        if (config[i] === '#') {
            if (i > 0 && !isNaN(config[i - 1])) { // Check if there's a number before the hash
                let imageIndex = parseInt(config[i - 1])-1; // Convert the character before the hash to an integer

                let triangleButton = document.querySelector(`.triangle-button[data-index='${imageIndex}']`);
                let imageWrapper = document.querySelector(`.image-wrapper[data-index='${imageIndex}']`);

                if (triangleButton && imageWrapper) {
                    imageWrapper.classList.add('active');
                    triangleButton.classList.add('extraSpace');
                } else {
                    console.log(`No button and/or image found for index ${imageIndex}`);
                }
            }
        }
    }

    // images and buttons are now formatted //

    for (const group of groups) { // for each group
        let groupLength = String(group.ID).length
        let option = 'None'; //default is None

        for (const savedMode of savedModes) { // for each saved mode
            if (savedMode.ID == group.savedModeID) { // if the saved mode is the corresponding one of the group
                if (possibleOptions[groupLength-1].includes(savedMode.name)) { // if the mode saved is still a valid option
                    option = savedMode.name; // set it to the option
                }

                break; // since we found the correct one, we can stop looking
            }
        }

        setTimeout(() => { // wait for the dropdowns to actually be made
            let groupID = `group-${group.ID}`
            let dropdown = document.querySelector(`.image-dropdown[groupID='${groupID}']`); // find the dropdown, then assign it the new saved option
            dropdown.value = option; // Set to default if no value is found, or it is now invalid
            selectedValues.set(groupID, dropdown.value)
        }, 1500); // 1500 milliseconds
    }
    
    // dropdowns are now correct //
}


///////////////////////////////////////////////////////////// We have now defined basic functions and event-listeners

document.addEventListener('DOMContentLoaded', function() { // this is called once when the page is first ready
    showLoadingScreen(); // show loading...
    hideWelcomeScreen();
    hideApiHelpScreen();
    setTimeout(() => {
        startUp();
    }, 1000); // Wait before calling startUp()
});


function startUp() {
    // Preload all images (1.jpg to 8.jpg)
    for (let i = 1; i <= maxImages; i++) {
        const imageUrl = `${i}.jpg`;
        preloadImages.push(imageUrl);
        preloadImage(imageUrl);
    }

    getSavedInfo()
        .then((savedInfo) => {
            // Parse the JSON data
            const data = JSON.parse(savedInfo);

            // Store the formatted data in variables
            const resetFlag = data.resetFlag;
            const config = data.configData.config;
            const groups = data.configData.groups;
            const override = data.configData.override;
            const savedModes = data.savedModes;
            const modes = data.modes;

            for (let i = 1; i <= 8; i++) {
                possibleOptions.push(getPossibleOptions(i, modes)); // load up all the possible options for each number of digits
            }

            createImage(); // Create the first image
            
            if (resetFlag === false) {                
                loadData(config, groups, savedModes, override);
                
                setTimeout(() => {
                    showSettingButtons(); // show the buttons as it was already saved
                }, 1500); // need to wait for groups and dropdowns to update

                setTimeout(() => {
                    hideLoadingScreen();
                }, 1000+1500); // you need to add the delay for the dropdown update too, currently 1.5sec

            } else { // reset flag is equal to true
                console.log('Reset flag is triggered');
                setTimeout(() => {
                    hideLoadingScreen();
                    showWelcomeScreen();
                }, 1500);
            }

            formattedData = { resetFlag, config, groups, savedModes, modes, override };
            
        })

        .catch((error) => {
            console.error('Error retrieving saved info:', error);
        });
}