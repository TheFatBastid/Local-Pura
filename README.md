<img src="pura logo.png">

## Summary

This allows for the use of the Pura Mini or Pura 4 diffuser with Home Assistant
via ESPhome. All Local control, no more cloud needed.

## Setup

1) Peel up the bottom of the silver back sticker and unscrew the two screws.
2) Connect to the 4x2 vias:
    * Your usb - serial converter's tx to pura's rx.
    * Your usb - serial converter's rx to pura's tx.
    * Your usb - serial converter's 3.3v to pura's 3.3v via
    * Your usb - serial converter's ground to pura's GND via.
    * Your usb - serial converter's ground to pura's I/O 0.
    
    For Pura Mini, it looks as below. For Pura 4 see the Pura 4 section.
    <img src="vias.png">
3) Download the stock firmware with : "esptool --port COM whatever_com_port_it_is read-flash 0 ALL pura-mini-backup.bin"
4) Use ESPHome dashboard to perform the initial flashing. Once you can OTA update it, you can disconnect the serial converter.
5) Note the blanked api encryption key, ota password, and fallback wifi password. Be sure that is all set.
7) Use samba or something to upload the st25r3918 component directory to HA, in /homeassistant/esphome/components/  .
9) Install the yaml on the device and enjoy.

I am a bad programmer, this will burn down your house, don't blame me.

## Limitations
1) Scent names come from their website, so you will need to build a local database. If you know of more please add them here!
2) Pura says they have some fancy-pants tech that diffuses different scents differently. Shrug. All I do is turn the heat to 3 different levels.
3) Made up my own calculations for % left based on: "With a Pura Mini, a fragrance vial lasts about 30 days in a small space, diffusing 6–8 hours per day at medium intensity."
4) This is only for the Mini right now. Someone send me the others to work on!
5) GPIO34 reads a divided down voltage but I didn't know what to do with that.
6) I can not find any connection that GPIO35 makes.
7) I am bad at programming.

   
Could not be done with the work of:
https://github.com/stm32duino/ST25R3916
and
https://github.com/stm32duino/ST25R3916

## Known Cart IDs

|   Cart ID | Name | Link |
| --------- | ---- | ---- |
|  E002080AA155AA6F | Lemon | https://pura.com/products/lemon |

## Pura Mini Layout
The esp32-wrover-e connects to a thermistor, ceramic heater, push button, 2 top leds, 1 button led, and a ST25R3918 NFC reader, which in turn connected to a Molex 14623605151 antenna.

esp32: https://documentation.espressif.com/esp32-wrover-e_esp32-wrover-ie_datasheet_en.html

ST25R3918: https://www.st.com/en/nfc/st25r3918.html A cut down STR253916.

Antenna: https://www.molex.com/en-us/products/part-detail/1462360151

Initial revision boards are white, Rev B boards are green. The only change seems
to be changing out the LNK3209G mosfet IC at U2 for the LNK3205D mosfet at U9
(and appropriate caps, etc)
<img src="revb pura mini.jpg">

| Pin | Mini Function | V4 function|
| --- | -------- | ------ |
| GPIO0| Held low for programming |Held low for programming|
| GPIO1| TX for programming |TX for programming|
| GPIO3| RX for programming |RX for programming|
| GPIO4| Push button state |Push button state|
|GPIO13| st25r3918 IRQ pin ||
|GPIO14| st25r3918 i2c clock pin ||
|GPIO15| LED near the push button ||
|GPIO21| Heater ||
|GPIO22| Top LEDS ||
|GPIO27| st25r3918 i2c data pin||
|GPIO34| Voltage sensing? ||
|GPIO35| Unknown ||
|GPIO36| Thermistor ||

## Pura 4

The larger Pura 4, with slots for 2 carts, has similar instructions but a different
layout. The pins for flashing are as shown. Once you have it working, you can
control the left and right carts independently in Home Assistant.

<img src="pura-4.png">

Use the pura-4.yaml file, and fill in config in a `secrets.yaml` file
following the examples in `secrets.yaml.example`

| Pin | Function |
| --- | -------- |
| GPIO0 | Held low for programming | 
| GPIO4 | Push button state |
| GPIO22| right heater |
| GPIO23| left heater |
| GPIO25| Top LEDs |
| GPIO26| NC |
| GPIO27| st25r3918 IRQ pin |
| GPIO32| st25r3918 i2c clock pin |
| GPIO33| st25r3918 i2c data pin |
| GPIO34| Unknown | 
| GPIO35| Unknown (shows connectivity on the upper light strip, but 25 is what you configure) | 
| GPIO36| right thermistor | 
| GPIO39| left thermistor |

### Pura 4 specific limitations

* The usage check doesn't really work since there are two cart slots
* The cart read only seems to work from the right slot


Support me here https://ko-fi.com/thefatbastid