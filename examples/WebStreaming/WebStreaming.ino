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

#include <TelitWiFi.h>
#include "config.h"

#include <Camera.h>
#include <time.h>

const uint16_t RECEIVE_PACKET_SIZE = 1500;
uint8_t Receive_Data[RECEIVE_PACKET_SIZE] = {0};

TelitWiFi gs2200;
TWIFI_Params gsparams;


/****************************************************************************
 * Initial parameters
 ****************************************************************************/
//#define USE_HDR_CAMERA
#define  CONSOLE_BAUDRATE  115200

int               g_width   = CAM_IMGSIZE_QVGA_H;
int               g_height  = CAM_IMGSIZE_QVGA_V;
CAM_IMAGE_PIX_FMT g_img_fmt = CAM_IMAGE_PIX_FMT_JPG;
int               g_divisor = 7;

CAM_WHITE_BALANCE g_wb      = CAM_WHITE_BALANCE_AUTO;
CAM_HDR_MODE      g_hdr     = CAM_HDR_MODE_ON; 

int               g_interval_times = 3; /*Seconds*/

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
  Init_GS2200_SPI_type(iS110B_TypeC);

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

#ifdef USE_HDR_CAMERA
  err = theCamera.setHDR(g_hdr);
  if (err != CAM_ERR_SUCCESS) { printError(err); }
#else
#endif

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

  char remote_cid = 0;
  char server_cid = 0;
  uint32_t timer = 5000;

  ConsoleLog( "Start TCP Server");

  server_cid = gs2200.start((char*)TCPSRVR_PORT);
  if (server_cid == ATCMD_INVALID_CID) {
    delay(2000);
    return;
  }

  while (1) {
    ConsoleLog( "Waiting for TCP Client");
    if (true != gs2200.wait_connection(&remote_cid, timer)) {
      continue;
    }

    ConsoleLog( "TCP Client Connected");
    sleep(1);
    unsigned long cam_before, cam_after, one_before, one_after;
    while (gs2200.available()) {
      if (0 < gs2200.read(remote_cid, Receive_Data, RECEIVE_PACKET_SIZE)) {
        String message = (char*)Receive_Data;

        int space1_pos = message.indexOf(' ');
        int space2_pos = message.indexOf(' ', space1_pos + 1);
        String method  = message.substring(0, space1_pos);
        String path    = message.substring(space1_pos + 1, space2_pos);
        //ConsolePrintf( "get method: %s\r\n", method.c_str() );
        //ConsolePrintf( "get path  : %s\r\n", path.c_str() );

        if (method == "GET" && path == "/cam.jpg") {

          String response = "HTTP/1.1 200 OK\r\n"
                            "Content-Type: multipart/x-mixed-replace; boundary=--MOBOTIX_Fast_Serverpush\r\n"
                            "\r\n";

          gs2200.write(remote_cid, (const uint8_t*)response.c_str(), response.length());
          while (1) {
            one_before = millis();

            cam_before = millis();
            CamImage img = theCamera.takePicture();
            cam_after = millis();
            ConsolePrintf( "Take Cam:%dms\n", cam_after - cam_before );

            if(img.getImgSize() != 0) {
              //Serial.print("picture is available:");
              //Serial.println(String(img.getImgSize()).c_str());

              response = "--MOBOTIX_Fast_Serverpush\r\n"
                        "Content-Length: " + String(img.getImgSize()) + "\r\n"
                        "Content-Type: image/jpeg\r\n"
                        "\r\n";

              gs2200.write(remote_cid, (const uint8_t*)response.c_str(), response.length());
              size_t sent_size = 0;
              for (sent_size = 0; sent_size < img.getImgSize();) {
                size_t send_size = min(img.getImgSize() - sent_size, MAX_RECEIVED_DATA - 100);
                bool result = false;
                do {
                  result = gs2200.write(remote_cid, (uint8_t *)(img.getImgBuff() + sent_size), send_size);
                  if (true == result) {
                    sent_size += send_size;
                  } else {
                    ConsolePrintf("Send Bulk Error, retry...\n");
                    delay(100);
                  }
                } while (true != result);
                delay(30);
              }

              one_after = millis();
              ConsolePrintf( "Send:%dms\n", one_after - one_before );

            } else {
              String response = "HTTP/1.1 404 Not Found";
              if (true != gs2200.write(remote_cid, (const uint8_t*)response.c_str(), response.length())) {
                delay(2000);
                return;
              }
            }

            response = "\r\n";
            gs2200.write(remote_cid, (const uint8_t*)response.c_str(), response.length());

            sleep(g_interval_times);
            WiFi_InitESCBuffer();
          }

        } else {
          // send HTTP Response
          String response = "HTTP/1.1 404 Not Found";
          if (true != gs2200.write(remote_cid, (const uint8_t*)response.c_str(), response.length())) {
            delay(2000);
            return;
          }
        }
        WiFi_InitESCBuffer();
      }
    }
  }
}
