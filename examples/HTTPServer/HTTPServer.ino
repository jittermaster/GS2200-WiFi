/*
 *  LimitedAPMode.ino - GainSpan WiFi Module Control Program
 *  Copyright 2022 Spresense Users
 *
 *  This work is free software; you can redistribute it and/or modify it under the terms 
 *  of the GNU Lesser General Public License as published by the Free Software Foundation; 
 *  either version 2.1 of the License, or (at your option) any later version.
 *
 *  This work is distributed in the hope that it will be useful, but without any warranty; 
 *  without even the implied warranty of merchantability or fitness for a particular 
 *  purpose. See the GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License along with 
 *  this work; if not, write to the Free Software Foundation, 
 *  Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <TelitWiFi.h>
#include "config.h"


#define  CONSOLE_BAUDRATE  115200

const uint16_t RECEIVE_PACKET_SIZE = 1500;
uint8_t Receive_Data[RECEIVE_PACKET_SIZE] = {0};

TelitWiFi gs2200;
TWIFI_Params gsparams;

// Contents data creation
String header         = " HTTP/1.1 200 OK \r\n";
String content        = "<!DOCTYPE html><html> Test!</html>\r\n";
String content_type   = "Content-type: text/html \r\n";
String content_length = "Content-Length: " + String(content.length()) + "\r\n\r\n";

int send_contents(uint8_t* ptr, uint16_t size)
{  
  String str = header + content_type + content_length + content;
  size = (str.length() > size)? size : str.length();
  str.getBytes(ptr, size);
  
  return str.length();
}

// the setup function runs once when you press reset or power the board
void setup() {

  /* initialize digital pin of LEDs as an output. */
  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  digitalWrite(LED0, LOW);   // turn the LED off (LOW is the voltage level)
  Serial.begin(CONSOLE_BAUDRATE); // talk to PC

  /* Initialize SPI access of GS2200 */
  Init_GS2200_SPI_type(iS110B_TypeC);

  /* Initialize AT Command Library Buffer */
  gsparams.mode = ATCMD_MODE_LIMITED_AP;
  gsparams.psave = ATCMD_PSAVE_DEFAULT;
  if( gs2200.begin(gsparams)) {
    ConsoleLog("GS2200 Initilization Fails");
    while(1);
  }

  /* GS2200 runs as AP */
  if( gs2200.activate_ap(AP_SSID, PASSPHRASE, AP_CHANNEL)) {
    ConsoleLog("WiFi Network Fails");
    while(1);
  }

  digitalWrite( LED0, HIGH ); // turn on LED

}

// the loop function runs over and over again forever
void loop() {

  char remote_cid = 0;
  char server_cid = 0;
  uint32_t timer = 5000;
  
  ConsoleLog("Start TCP Server");
  server_cid = gs2200.start_tcp_server((char*)TCPSRVR_PORT);
  if (server_cid == ATCMD_INVALID_CID) {
    delay(2000);
    return;
  }

  while(1) {
    ConsoleLog("Waiting for TCP Client");
    if (true != gs2200.wait_connection(&remote_cid, timer)) {
      continue;
    }

    ConsoleLog("TCP Client Connected");
    sleep(1);

    // Start the echo server
    while (gs2200.available()) {
      WiFi_InitESCBuffer();
      if (0 < gs2200.read(remote_cid, Receive_Data, RECEIVE_PACKET_SIZE)) {
          ConsolePrintf("Received : %s\r\n", Receive_Data);
          String message = (char*)Receive_Data;
          if (message.substring(0, message.indexOf(' ')) == "GET") {
            int length = send_contents(Receive_Data, RECEIVE_PACKET_SIZE);
            ConsolePrintf("Will send : %s\r\n", Receive_Data);
            if (true != gs2200.write(remote_cid, Receive_Data, length)) {
              // Data is not sent, we need to re-send the data
              delay(10);
              ConsolePrintf("Sent Error : %s\r\n", Receive_Data);
            }
          }
      }
    }
  }
}
