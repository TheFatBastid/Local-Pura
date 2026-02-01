This allows for the use of the Pura Mini diffuser with Home Assistant via ESPhome. All Local control, no more cloud needed.

1) Peel up the bottom of the silver back sticker and unscrew the two screws.
2) Use the 4x2 vias to back up the stock firmware and reflash with esphome.
3) Put the yaml on the device, do the keys and whatnot.
4) Use samba or something to upload the component dir to HA, in esphome/components. (or tediously upload one file at a time with the File Editor add-on)
5) Install and enjoy.

I am a bad programmer, this will burn down your house, don't blame me.

Limitations:
1) Scent names come from their website, so you will need to build a local databse. If you know of more please add them here!
2) Pura says they have some fancy-pants tech that diffuses different scents differently. Shrug. All I do is turn the heat to 3 different levels.
3) Made up my own calculations for % left based on what they say an average scent should last at medium 8/h use.
4) This is only for the Mini right now. Someone send me the others to work on!

Could not be done with the work of:
https://github.com/stm32duino/ST25R3916
and
https://github.com/stm32duino/ST25R3916

|   Cart ID | Name | Link |
| --------- | ---- | ---- |
|  E002080AA155AA6F | Lemon | https://pura.com/products/lemon |

<img src="revb pura mini.jpg">

Support me here https://ko-fi.com/thefatbastid
