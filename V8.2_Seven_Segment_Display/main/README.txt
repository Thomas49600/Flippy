This is V8.2 of a mechanical seven-segment designed to display both words and numbers in an attractive and interesting way.
This code supports up to 8 digits and has room for expansion in the future

To upload this code do the following:
> Hold down the "flash" button on your NodeMCU
> Plug the USB connector into your computer and NodeMCU
> Wait 3 seconds
> Release the "flash" button

> Open wifi.cpp in a text editor and add your local wifi details on line 67 of wifi.cpp (make sure they are 2.4g)
> Open main.ino in Arduino IDE then compile + upload it using Arduino IDE or similar
	> Watch the following video (Credit to Brian Lough) on how to install Arduino IDE, the ESP8266 Software and the required drivers
		> https://www.youtube.com/watch?v=AFUAMVFzpWw

	> Choose the board "NodeMCU 1.0 (ESP-12E Module)"
	> Under "Tools" (top menu) choose "Flash Size - 4MB (FS:3MB OTA:~512KB)" and "Upload Speed: 115200"
	> Then hit upload (arrow button ->)


After uploading the code you will need to upload the file system
> Follow the instructions listed on this page to install the file system plugin (this is for Arduino IDE 2.2.1 or higher)
	> https://github.com/earlephilhower/arduino-littlefs-upload (Credit to earlephilhower)

> There are unfortunately a few bugs with this plugin
	> If you attempt to upload and it says "Please compile this sketch at least once", select a random board and attempt to upload the files (it will fail) then go back to your NodeMCU board and try again (it should work)
	> If you get a file path "Undefined" when you attempt to upload, close and re-open your sketch
	> If it says it cannot open serial port <port>, make sure you have closed serial monitor in all instances of Arduino IDE

> Ensure you are connected to your NodeMCU via USB and the NodeMCU is running normally (this will also likely require 12V to be plugged into the main PCB and the common anode digit connector to be disconnected)
> Then simply press [Ctrl] + [Shift] + [P], then "Upload LittleFS to Pico/ESP8266/ESP32"


After restarting your NodeMCU (either by disconnecting and reconnecting power or the RST button) you can now access the main page at "flippy.local" in a web browser on the same wifi network as you connected the device to
Sometimes due to lag the pages may fail to properly load, if this happens, refresh the webpage and it should work.
The webpages are intended for computers so if you wish to use them on your phone, simply select "desktop site" in your browser.
