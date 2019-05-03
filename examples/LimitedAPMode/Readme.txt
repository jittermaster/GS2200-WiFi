Change MACRO in config.h

- AP_SSID : SSID of GS2200 Access Point
- PASSPHRASE : Passphrase of AP WPA2 security
- TCPSRVR_PORT : TCP Server port number


After running this example, your PC should associate to GS2200.

Then, run "node tcp_client.js". tcp_client is in script directory.

tcp_client.js receives the data from stndard input and sends it to GS2200 TCP server.

GS2200 TCP server echoes back the data to TCP client, so tcp_client.js will display the echo back data.

You need to install Node.js on your computer.

