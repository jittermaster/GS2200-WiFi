< Change MACRO in config.h >

- AP_SSID      : SSID of WiFi Access Point to connect
- PASSPHRASE   : Passphrase of AP WPA2 security
- HTTP_SRVR_IP : HTTP Server IP Address
- HTTP_PORT    : HTTP Server port number


< PostTest.ino >

After connecting HTTP server, the sample code continues to send 10000 byte POST data in 1 sec interval.

You can change the data size of the POST data, but the variable "size_string" should be same as new data size of the POST data, and AtCmd_HTTPCONF( HTTP_HEADER_CONTENT_LENGTH, size_string ) should run before AtCmd_HTTPSEND().


< Post_Test.js >

Before running this example, you should run "Post_Test.js". Post_Test.js is in the script directory is the sample code of Node.js HTTP server.

When finishing the POST request data reception, Post_Test.js shows the data size of POST data.

You need to install Node.js on your computer, and run following.

node Post_Test.js


< TelitWiFi Class >

TelitWiFi class supports following configuration.

TWIFI_Params params;

params.psave = ATCMD_PSAVE_DEFAULT; --> Power save mode  
params.psave = ATCMD_ALWAYS_ON; --> Never sleep

params.mode = ATCMD_MODE_STATION; --> Station mode
params.mode = ATCMD_MODE_LIMITED_AP; --> AP mode

In case of AP mode, use following member function.

int connect(const char *ssid, const char *passphrase, uint8_t channel);
