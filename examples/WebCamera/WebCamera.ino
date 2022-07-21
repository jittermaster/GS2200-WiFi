/*
 *  WebCamera.ino - Web Camera via GS2200
 *  Copyright 2020 Spresense Users
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

#include <Camera.h>
#include <time.h>

extern uint8_t ESCBuffer[];
extern uint32_t ESCBufferCnt;

TelitWiFi gs2200;
TWIFI_Params gsparams;

String ssid(AP_SSID);
String pass(PASSPHRASE);
String ip(AP_SSID);
String port(PASSPHRASE);

/****************************************************************************
 * Initial parameters
 ****************************************************************************/
#define  CONSOLE_BAUDRATE  115200

int               g_width   = CAM_IMGSIZE_VGA_H;
int               g_height  = CAM_IMGSIZE_VGA_V;
CAM_IMAGE_PIX_FMT g_img_fmt = CAM_IMAGE_PIX_FMT_JPG;
int               g_divisor = 7;

CAM_WHITE_BALANCE g_wb      = CAM_WHITE_BALANCE_AUTO;
CAM_HDR_MODE      g_hdr     = CAM_HDR_MODE_ON; 

/****************************************************************************
 * Print error message
 ****************************************************************************/
void printError(enum CamErr err)
{
  Serial.print("Error: ");
  switch (err) {
  case CAM_ERR_NO_DEVICE:             Serial.println("No Device");                     break;
  case CAM_ERR_ILLEGAL_DEVERR:        Serial.println("Illegal device error");          break;
  case CAM_ERR_ALREADY_INITIALIZED:   Serial.println("Already initialized");           break;
  case CAM_ERR_NOT_INITIALIZED:       Serial.println("Not initialized");               break;
  case CAM_ERR_NOT_STILL_INITIALIZED: Serial.println("Still picture not initialized"); break;
  case CAM_ERR_CANT_CREATE_THREAD:    Serial.println("Failed to create thread");       break;
  case CAM_ERR_INVALID_PARAM:         Serial.println("Invalid parameter");             break;
  case CAM_ERR_NO_MEMORY:             Serial.println("No memory");                     break;
  case CAM_ERR_USR_INUSED:            Serial.println("Buffer already in use");         break;
  case CAM_ERR_NOT_PERMITTED:         Serial.println("Operation not permitted");       break;
  default:
    break;
  }
  
  exit(1);

}

/****************************************************************************
 * setup
 ****************************************************************************/
void setup()
{
  /* initialize digital pin of LEDs as an output. */
  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  ledOn(LED0);
  Serial.begin( CONSOLE_BAUDRATE );

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

  Serial.println("Setup Camera...");

  CamErr err = theCamera.begin();
  if (err != CAM_ERR_SUCCESS) { printError(err); }

//  err = theCamera.startStreaming(false, (void*)0 );
//  if (err != CAM_ERR_SUCCESS) { printError(err); }

  err = theCamera.setHDR(g_hdr);
  if (err != CAM_ERR_SUCCESS) { printError(err); }

  err = theCamera.setAutoWhiteBalanceMode(g_wb);
  if (err != CAM_ERR_SUCCESS) { printError(err); }

  err = theCamera.setStillPictureImageFormat(g_width, g_height, g_img_fmt, g_divisor);
  if (err != CAM_ERR_SUCCESS) { printError(err); }

  Serial.println("Setup Camera done.");

  ledOn(LED0);
}

/****************************************************************************
 * setup
 ****************************************************************************/
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

  while( 1 ) {
    ConsoleLog( "Waiting for TCP Client");

    if( ATCMD_RESP_TCP_SERVER_CONNECT != WaitForTCPConnection( &remote_cid, 5000 ) ) {
      continue;
    }

    ConsoleLog( "TCP Client Connected");
    // Prepare for the next chunck of incoming data
    WiFi_InitESCBuffer();
    sleep(1);
    
    unsigned long cam_before, cam_after, one_before, one_after;
    while( Get_GPIO37Status() ) {
      resp = AtCmd_RecvResponse();
      if( ATCMD_RESP_BULK_DATA_RX == resp ) {
        if( Check_CID( remote_cid ) ) {

          String message = ESCBuffer+1;

          int space1_pos = message.indexOf(' ');
          int space2_pos = message.indexOf(' ', space1_pos + 1);
          String method  = message.substring(0, space1_pos);
          String path    = message.substring(space1_pos + 1, space2_pos);
          //ConsolePrintf( "get method: %s\r\n", method.c_str() );
          //ConsolePrintf( "get path  : %s\r\n", path.c_str() );

          if (method == "GET" && path == "/cam.jpg") {
            one_before = millis();

            cam_before = millis();
            CamImage img = theCamera.takePicture();
            cam_after = millis();
            ConsolePrintf( "Take Cam:%dms\n", cam_after - cam_before );

            if(img.getImgSize() != 0) {
              //Serial.print("picture is available:");
              //Serial.println(String(img.getImgSize()).c_str());

              String response = "HTTP/1.1 200 OK\r\n"
                                "Content-Type: image/jpeg\r\n"
                                "Content-Length: " + String(img.getImgSize()) + "\r\n"
                                "\r\n";

              ATCMD_RESP_E err = AtCmd_SendBulkData( remote_cid, response.c_str(), response.length() );

              size_t sent_size = 0;
              for (size_t sent_size = 0; sent_size < img.getImgSize();) {
                size_t send_size = min(img.getImgSize() - sent_size, MAX_RECEIVED_DATA - 100);

                ATCMD_RESP_E err = AtCmd_SendBulkData( remote_cid, (uint8_t *)(img.getImgBuff() + sent_size), send_size );
                if (ATCMD_RESP_OK == err) {
                  sent_size += send_size;
                } else {
                  ConsolePrintf( "Send Bulk Error, %d\n", err );
                  delay(2000);
                  return;
                }
                delay(30);

              }
              one_after = millis();
              ConsolePrintf( "Send:%dms\n", one_after - one_before );

            } else {
              String response = "HTTP/1.1 404 Not Found";
              ATCMD_RESP_E err = AtCmd_SendBulkData( remote_cid, response.c_str(), response.length() );
              if (ATCMD_RESP_OK != err) {
                ConsolePrintf( "Send Bulk Error, %d\n", err );
                delay(2000);
                return;
              }
            }
          } else {
            // send HTTP Response
            String response = "HTTP/1.1 404 Not Found";
            ATCMD_RESP_E err = AtCmd_SendBulkData( remote_cid, response.c_str(), response.length() );
            if (ATCMD_RESP_OK != err) {
              ConsolePrintf( "Send Bulk Error, %d\n", err );
              delay(2000);
              return;
            }
          }
        }

        WiFi_InitESCBuffer();

      }
    }
  }
}
