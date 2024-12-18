import { getUrlData, sendToDisplayAPI } from './modes.js';

const urlData = getUrlData(); // this will also update the group ID text


function hideButtonsByValue(threshold) {
    // Select all buttons
    const buttons = document.querySelectorAll('button[button-value]');
    
    // Iterate through each button
    buttons.forEach(button => {
        const buttonValue = parseInt(button.getAttribute('button-value'), 10);
        if (Math.abs(buttonValue) > threshold) {
            button.classList.add('hide'); // Apply the 'hide' class
        } else {
            button.classList.remove('hide'); // Ensure the class is removed if not greater
        }
    });
}


// Attach event listener to both plusButtons and minusButtons containers
document.getElementById('plusButtons').addEventListener('click', handleButtonClick);
document.getElementById('minusButtons').addEventListener('click', handleButtonClick);

document.getElementById('addButton').addEventListener('click', () => handleCustomAmount('add'));
document.getElementById('subtractButton').addEventListener('click', () => handleCustomAmount('subtract'));


let currentValue = 0;
let oldValue = 9999999999;
const maxValue = (10 ** String(urlData.groupID).length) - 1;
const minValue = Math.trunc(maxValue/10)*-1;

hideButtonsByValue(maxValue+1);

//console.log(`max value ${maxValue}, min value ${minValue}`);

function updateCurrentValue(amount) {
    currentValue += amount; // this will subtract if the action is negative

    if (currentValue >= maxValue) {
        currentValue = maxValue;
    
    } else if (currentValue <= minValue) {
        currentValue = minValue;
    }

    if (currentValue == -0) {
        currentValue = 0;
    }

    if (currentValue != oldValue) { // if it has changed
        oldValue = currentValue;

        const jsonData = {
            groupID: urlData.groupID, // Use the retrieved groupID from the URL
            content: currentValue,
            type: 'text',
            align: 'R',
            mode: urlData.modeName // Use the modeName from the URL parameters
        };
    
        sendToDisplayAPI(jsonData);
    }
}

function handleButtonClick(event) {
    if (event.target.tagName === 'BUTTON') { // Ensure a button was clicked
        const action = event.target.getAttribute('button-value');
        updateCurrentValue(Number(action));        
    }
}

function handleCustomAmount(action) {
    let content = Number(document.getElementById('inputField').value);  // Input field for content

    if (isNaN(content)) {
        alert(`That's not a number...`);
        return;
    }

    if (!(Number.isInteger(content) && content >= 0)) { // if it isn't a positive integer
        alert(`${content} is not a positive integer!`);
        return;
    }
    
    if (action == 'subtract') {
        content = -1*content;
    }

    updateCurrentValue(content);
}

