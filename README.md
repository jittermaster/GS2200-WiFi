# GS2200-WiFi

[GS2200 WiFi add-on board](https://idy-design.com/product/is110b.html) operation

![image](https://raw.githubusercontent.com/jittermaster/GS2200-WiFi/master/IS110B.jpg)

## Description

SPRESENSE-WiFi is the sample program to kick GS2200 for the WiFi data transfer.

## Features

- TCP Client : After connecting TCP server, it sends data on and on....

## Requirement

- Arduino IDE

## Usage

Change MACRO in AppFunc.cpp.
- AP_SSID : SSID of WiFi Access Point to connect
- PASSPHRASE : Passphrase of AP WPA2 security
- TCPSRVR_IP : TCP Server IP Address
- TCPSRVR_PORT : TCP Server port number

## Author

Norikazu Goto

## License

[LGPL](http://www.gnu.org/licenses/lgpl.html)

## Histroy

3/5/2019 : 1.0.0

