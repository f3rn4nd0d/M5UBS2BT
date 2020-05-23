# M5UBS2BT

## About
This project enables you to connect your favorite USB connected HID devices with Bluetooth.\
Currently, only Keyboard HID is supported. Mouse HID will be supported.

[![demo](https://pbs.twimg.com/ext_tw_video_thumb/1263056500295008256/pu/img/HI9dPeKaTurj9ydT?format=jpg&name=small)](https://twitter.com/shinoalice_kabo/status/1263056543794130944)

## Required
The following hardware is required.
* M5Stack Basic or M5Stack Gray
* [Module USB using MAX3421E](https://docs.m5stack.com/#/en/module/usb)
* Your favarite USB keyboard.

## Build
1. Follow instructions on the below link to setup Arduino for M5Stack.

    https://github.com/m5stack/M5Stack

2. Please add the following library．

    https://github.com/m5stack/M5-ProductExampleCodes/tree/master/Module/USB/Arduino

3. Clone this project and compile M5UBS2BT.ino and write to M5Stack. 

## Usage
The keyboard connected with Module-USB of M5Stack is operated as a Bluetooth keyboard.\
Please add a Bluetooth device on your PC or smartphone.\
The device name is indicated by the `M5Stack_NAME` defined in config.h.\
While connected to the device, `Connected` will be displayed on the M5Stack screen.

In addition, the following extensions are supported.

* Send any string.
    * Please add your favorite character string to `PASSWORD` in config.h.
* Enables/disables continuous string transmission by pressing and holding the key.
    * If you disabled this function, please comment out `LONG_PRESS` in config.h.

## Test Environment

* Android 10
* Windows 10 pro 2004

## Restriction

* Only the keyboard is supported.
* Connection and switching with multiple devices are not supported. Please switch to a connected device.

## Contact / Troubleshooting

If you have any problems or requests, please contact me. ( English or Japanese)\
twitter @shinoalice_kabo

## Related Projects

* Original : https://github.com/mhama/M5StackHIDCtrlAltDel