# GoogleFormPost
Arduino / Platform.io library to add data to Google Spreadsheets via Google Forms, without API or authentication.
Specifically for ESP-8266 and ESP32 platforms.

[MIT license](LICENSE) / (c) 2023 [John Mueller](https://johnmu.com/)

![Release](https://img.shields.io/github/v/release/softplus/GoogleFormPost?include_prereleases) |
[![arduino-library-badge](https://www.ardu-badge.com/badge/GoogleFormPost.svg?)](https://www.ardu-badge.com/GoogleFormPost) |
[![Build with PlatformIO](https://img.shields.io/badge/PlatformIO-Library-orange?)](https://platformio.org/lib/show/567/GoogleFormPost/installation)

*This works with the ESP8266 Arduino platform* - https://github.com/esp8266/Arduino

*This works with the ESP32 Arduino platform* - https://github.com/espressif/arduino-esp32

# Usage

## Installing
You can either install through the Arduino Library Manager or checkout the latest changes or a release from github. 
To use this, you *must* also set up a Google Form and Spreadsheet (below).

### Install through Library Manager
__Currently version 0.8+ works with release 2.4.0 or newer of the [ESP8266 core for Arduino](https://github.com/esp8266/Arduino)__
 - in Arduino IDE got to Sketch/Include Library/Manage Libraries
  ![Manage Libraries](http://i.imgur.com/9BkEBkR.png)

 - search for GoogleFormPost
  ![GoogleFormPost package](http://i.imgur.com/18yIai8.png)

 - click Install and start [using it](#using)

###  Checkout from github
__Github version works with release 2.4.0 or newer of the [ESP8266 core for Arduino](https://github.com/esp8266/Arduino)__
- Checkout library to your Arduino libraries folder

## Example code

Example code is in `/examples/`.

```c++
#include "GoogleFormPost.h"

// ... connect to your Wifi, get data, etc
// ... when you're ready ...

#define YOUR_FORM_URL "https://docs.google.com/forms/d/e/1FA..."
#define FIELD_ID_1 "entity.12345678"
#define FIELD_ID_2 "entity.109876543"

    String yourDataString = ...;
    String yourOtherDataString = ...;

    GoogleFormPost gf;
    gf.setFormUrl( YOUR_FORM_URL );
    gf.addData( yourDataString, FIELD_ID_1 );
    gf.addData( yourOtherDataString, FIELD_ID_2 );
    gf.send();
    // done! Total time ca 2 seconds

// Alternately, if you don't have the field IDs, you can read them:

    GoogleFormPost gf;
    gf.setFormUrl( YOUR_FORM_URL );
    gf.readFields();
    gf.addData( yourDataString );
    gf.addData( yourOtherDataString );
    gf.send();
    // done! Total time ca 6-7 seconds

```

## Setting up your Form and Spreadsheet

To submit to your own form and spreadsheet, you have to do the following:

1. Create a new Google Form ( https://form.new/ works).
2. Add a few "short answer" fields (however many you want).
3. Click on "Responses" and "Link to Sheets" to create a spreadsheet.
4. Click on the eyeball-symbol ("preview") to open the form.
5. Copy & paste that URL into your code.
6. That's it.

# Usage notes

Comments:

* Use the specified form URL; does not support redirects
* Reading form fields takes about 4.5s on an ESP01 (ESP8266); be faster by supplying the fields yourself. The code is not super-efficient, but primarily reading from wifi is slow with these devices.
* Submitting form data takes about 1.6s on an ESP01 (ESP8266)
* Code uses Arduino-style "String" variables; can be converted to std::string if you hate Strings.
