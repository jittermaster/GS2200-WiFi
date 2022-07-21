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

#include <GS2200Hal.h>
#include <GS2200AtCmd.h>
#include <TelitWiFi.h>
#include "config.h"


#define  CONSOLE_BAUDRATE  115200


extern uint8_t ESCBuffer[];
extern uint32_t ESCBufferCnt;

TelitWiFi gs2200;
TWIFI_Params gsparams;

// Contents data creation
String header         = " HTTP/1.1 200 OK \r\n";
String content        = "<!DOCTYPE html><html> Test!</html>\r\n";
String content_type   = "Content-type: text/html \r\n";
String content_length = "Content-Length: " + String(content.length()) + "\r\n\r\n";

int send_contents(char* ptr, int size)
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

  digitalWrite( LED0, LOW );   // turn the LED off (LOW is the voltage level)
  Serial.begin( CONSOLE_BAUDRATE ); // talk to PC

  /* Initialize SPI access of GS2200 */
  Init_GS2200_SPI();

  /* Initialize AT Command Library Buffer */
  gsparams.mode = ATCMD_MODE_LIMITED_AP;
  gsparams.psave = ATCMD_PSAVE_DEFAULT;
  if( gs2200.begin( gsparams ) ){
    ConsoleLog( "GS2200 Initilization Fails" );
    while(1);
  }

  /* GS2200 runs as AP */
  if( gs2200.activate_ap( AP_SSID, PASSPHRASE, AP_CHANNEL ) ){
    ConsoleLog( "WiFi Network Fails" );
    while(1);
  }

  digitalWrite( LED0, HIGH ); // turn on LED

}

// the loop function runs over and over again forever
void loop() {

  ATCMD_RESP_E resp;
  char server_cid = 0, remote_cid=0;
  ATCMD_NetworkStatus networkStatus;
  uint32_t timer=0;


  resp = ATCMD_RESP_UNMATCH;
  ConsoleLog( "Start TCP Server");
  
  resp = AtCmd_NSTCP( TCPSRVR_PORT, &server_cid);
  if (resp != ATCMD_RESP_OK) {
    ConsoleLog( "No Connect!" );
    delay(2000);
    return;
  }

  if (server_cid == ATCMD_INVALID_CID) {
    ConsoleLog( "No CID!" );
    delay(2000);
    return;
  }

  while( 1 ){
    ConsoleLog( "Waiting for TCP Client");

    if( ATCMD_RESP_TCP_SERVER_CONNECT != WaitForTCPConnection( &remote_cid, 5000 ) ){
      continue;
    }

    ConsoleLog( "TCP Client Connected");
    // Prepare for the next chunck of incoming data
    WiFi_InitESCBuffer();
    sleep(1);

    // Start the echo server
    while( Get_GPIO37Status() ){
      resp = AtCmd_RecvResponse();

      if( ATCMD_RESP_BULK_DATA_RX == resp ){
        if( Check_CID( remote_cid ) ){
          ConsolePrintf( "Received : %s\r\n", ESCBuffer+1 );

          String message = ESCBuffer+1;
          if (message.substring(0, message.indexOf(' ')) == "GET") {
            int length = send_contents(ESCBuffer+1,MAX_RECEIVED_DATA);        
            ConsolePrintf( "Will send : %s\r\n", ESCBuffer+1 );
            if( ATCMD_RESP_OK != AtCmd_SendBulkData( remote_cid, ESCBuffer+1, length ) ){
              // Data is not sent, we need to re-send the data
              delay(10);
              ConsolePrintf( "Sent Error : %s\r\n", ESCBuffer+1 );
            }
          }
        }          
        WiFi_InitESCBuffer();
      }
    }
  }
}
