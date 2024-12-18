import { getUrlData, sendToDisplayAPI } from './modes.js';

const urlData = getUrlData(); // this will also update the group ID text


document.querySelector(".contact-form3-form").addEventListener("submit", function(event) {
    // Prevent the default button click behavior
    event.preventDefault();

    // Collect data from the form fields
    const content = document.querySelector(".input").value;  // Input field for content
    const type = document.querySelector(".contact-form3-select1").value; // Select field for content type
    const align = document.querySelectorAll(".contact-form3-select1")[1].value;  // Select field for content alignment

    // Prepare the JSON object to send
    const jsonData = {
        groupID: urlData.groupID, // Use the retrieved groupID from the URL
        content: content,
        type: type,
        align: align,
        mode: urlData.modeName // Use the modeName from the URL parameters
    };

    sendToDisplayAPI(jsonData);
});