Change MACRO in config.h

- AP_SSID : SSID of WiFi Access Point to connect
- PASSPHRASE : Passphrase of AP WPA2 security
- TCPSRVR_IP : TCP Server IP Address
- TCPSRVR_PORT : TCP Server port number


Before running this example, you should run the TCP server.

tcp_server.js in script directory is the sample code of Node.js TCP server.

tcp_server.js calculates the receiving throughput. If you want to incrase the throughput, set the longer data in TCP_Data[],
but don't exceed 1460 byte. 

You need to install Node.js on your computer, and run the fillowing command.

node tcp_server.js

