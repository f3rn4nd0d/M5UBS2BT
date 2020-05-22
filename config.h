#ifndef CONFIG_H
#define CONFIG_H

//The name of the Bluetooth device. You can choose any one you like!
#define M5Stack_NAME "M5-BTKBD-Kabocha"

//Disabling this option will disable key press events.
#define LONG_PRESS
//This is option. If you don't like these, please change these.
#ifdef LONG_PRESS
#define START_COUNT (5)
#define WAIT (50)
#define INTERVAL (50)
#endif

//When you press the B button on M5Stack, the character string specified in PASSWORD is sent.
#define PASS_UNLOCK
#ifdef PASS_UNLOCK
#define PASSWORD "Your PC password\n"
#endif

//Define or Change if necessary
#define US_KEYBOARD
#define SERIAL_SPEED (115200)
//#define dobogusinclude

// UnSupported. The author will do best.
//#define MOUSE_SUPPORT

#endif
