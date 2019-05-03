< Change MACRO in config.h >

- AP_SSID      : SSID of WiFi Access Point to connect
- PASSPHRASE   : Passphrase of AP WPA2 security
- HTTP_SRVR_IP : HTTP Server IP Address
- HTTP_PORT    : HTTP Server port number

< HTTP_server.js >

Before running this example, you should run the HTTP server and confirm the IP address of HTTP server.

HTTP_server.js in script directory is the sample code of Node.js HTTP server.

HTTP_server.js recoginzes the POST request data in format "data=xxxx", keeps the latest one, and sends it back when receiving GET request.

You need to install Node.js on your computer, and run following.

node HTTP_server.js


< TelitWiFi Class >

TelitWiFi class supports following configuration.

TWIFI_Params params;

params.psave = ATCMD_PSAVE_DEFAULT; --> Power save mode  
params.psave = ATCMD_ALWAYS_ON; --> Never sleep

params.mode = ATCMD_MODE_STATION; --> Station mode
params.mode = ATCMD_MODE_LIMITED_AP; --> AP mode

In case of AP mode, use following member function.

int connect(const char *ssid, const char *passphrase, uint8_t channel);


< Power Save Mode >

In most cases, the power save mode is controlled by AT+WRXACTIVE and AT+WPXPS. If AT+WRXACTIVE=1, AT+WRXPS does not matter.

Power save mode:
  AT+WRXACTIVE=0 && AT+WRXPS=1

Never sleep mode:
  AT+WRXACTIVE=1

