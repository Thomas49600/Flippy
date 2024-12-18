import { getUrlData, sendToDisplayAPI } from './modes.js';

const urlData = getUrlData(); // this will also update the group ID text

// JavaScript to update diceFaces label with the value of diceSlider
const diceSlider = document.getElementById("diceSlider");
const diceFaces = document.getElementById("diceFaces");
const rollButton = document.getElementById('rollButton');
const checkBox = document.getElementById("showResults")


diceSlider.max = (10 ** urlData.groupID.length) - 1;

// Update diceFaces label whenever the slider value changes
diceSlider.addEventListener("input", () => {
  diceFaces.textContent = diceSlider.value;
});


function getRandomInt(min, max) {
    return Math.floor(Math.random() * (max - min + 1)) + min;
}

// Add event listener to the button for the 'click' event
rollButton.addEventListener('click', (event) => {
    event.preventDefault(); // Prevents the default form submission behavior

    const align = document.querySelector('.contact-form3-select1').value;

    let result = getRandomInt(1, diceSlider.value); // "roll" the dice
    console.log(result);

    // Prepare the JSON object to send
    const jsonData = {
        groupID: urlData.groupID, // Use the retrieved groupID from the URL
        content: result,
        type: "num",
        align: align,
        mode: urlData.modeName // Use the modeName from the URL parameters
    };

    sendToDisplayAPI(jsonData);

    if (checkBox.checked) {
        alert("You rolled a " + result);
    }
});