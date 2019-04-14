Change MACRO in config.h

- AP_SSID : SSID of WiFi Access Point to connect
- PASSPHRASE : Passphrase of AP WPA2 security

You need to install DSP codec binary to SPRESENSE

https://developer.sony.com/develop/spresense/developer-tools/get-started-using-arduino-ide/developer-guide#_dsp_codec_files

DSP codec will be installed to SD card or SPI flash.
If it is installed to SPI flash, you need to change the code to "/mnt/spif/BIN" in NetRadio.ino.

Enjoy music!

