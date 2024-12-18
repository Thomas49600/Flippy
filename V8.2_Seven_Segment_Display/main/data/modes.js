export { getUrlData, sendToDisplayAPI, clearDisplay };

// Function to get the parameters from the URL and return as an object
function getUrlData() {
    // Get the URL parameters
    const urlParams = new URLSearchParams(window.location.search);
    const encodedData = urlParams.get('p'); // Retrieve the encoded data from the URL
    let groupID, savedModeID, modeName;

    if (encodedData) {
        // Decode the Base64 string back into a JSON string
        const jsonString = atob(encodedData); // atob() decodes the Base64 string

        // Parse the JSON string back into an object
        const urlData = JSON.parse(jsonString);

        // Access the values
        groupID = urlData.groupID;
        savedModeID = urlData.savedModeID; // This is not directly used but can be retained for reference
        modeName = urlData.modeName;
        console.log(`Group ID: ${groupID}, Saved Mode ID: ${savedModeID}, Mode Name: ${modeName}`);

        // set the groupID text to the group ID
        const groupIDTextElement = document.getElementById('groupID-text');
        if (groupIDTextElement) {
            groupIDTextElement.textContent = groupID; // Set the groupID as the text content
        }

        return urlData;
    } else {
        // Return null if no encoded data is found
        return null;
    }
}

function sendToDisplayAPI(jsonData) {
    console.log(jsonData);
    
    // Send the JSON data as a POST request to /esp-apis/display
    fetch('/esp-apis/display', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify(jsonData),
    })
    .then(response => {
        if (!response.ok) {
            throw new Error('Network response was not ok');
        }
        return response.text();
    })
    .then(data => {
        console.log('Success:', data);
    })
    .catch((error) => {
        console.error('Error:', error);
    });
}


function clearDisplay() {
    const numOfDigits = String(urlData.groupID).length;
    const char = "#"; // used as a marker to clear
    const repeatedChar = char.repeat(numOfDigits);

    const clearScreen = {
        groupID: urlData.groupID, // Use the retrieved groupID from the URL
        content: repeatedChar,
        type: "text",
        align: "L",
        mode: urlData.modeName // Use the modeName from the URL parameters
    };

    sendToDisplayAPI(clearScreen);
}