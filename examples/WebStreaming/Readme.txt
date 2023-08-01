Change MACRO in config.h

- AP_SSID : SSID of GS2200 Access Point
- PASSPHRASE : Passphrase of AP WPA2 security
- TCPSRVR_PORT : TCP Server port number

Connect your Spresense Camera Board to Spresense Main Board.

After running this example, your PC should associate to GS2200 AP.

Then,

a. If your OS is Windows, please input WIN + R and open CMD, type "ipconfig" to confirm the ip address of GS2200.

b. If your OS is Linux, please open terminal and type "ip address" to confirm the ip address of GS2200.

Assuming that the ip address of GS2200 is 192.168.1.99, then open your browser and access 192.168.1.99/cam.jpg,

the image photographed by your camera will be displayed and updated on the web page.