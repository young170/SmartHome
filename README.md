## SmartHome

### Requirements
The project was developed and tested on the `nRF-52840-DK` development kit.

### Overview
When connected, the mobile application on the connected device can display the received button state and control the state of **LED 3** on the development kit.

### User Interface
LED 1:
   Blinks when the main loop is running (that is, the device is advertising) with a period of two seconds, duty cycle 50%.

LED 2:
   Lit when the development kit is connected.

LED 3:
   Lit when the development kit is controlled remotely from the connected device.

Button 1:
   Send a notification with the button state: "pressed" or "released".

### Testing
After programming the to your development kit, test it by performing the following steps:

1. Start the `nRF Connect for Mobile`_ application on your smartphone or tablet.
2. Power on the development kit.
3. Connect to the device from the application.<br>
   The device is advertising as `SmartHome`.<br>
   The services of the connected device are shown.
4. Write the following values to the LED characteristic in the **Nordic LED Button Service**.

| Hardware platform   | Value  | Effect                             |
|---------------------|--------|------------------------------------|
| Development kit     | `OFF`  | Switch the **LED3** off.           |
|                     | `ON`   | Switch the **LED3** on.            |
