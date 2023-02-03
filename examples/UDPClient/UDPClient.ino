/*
 *  SPRESENSE_WiFi.ino - GainSpan WiFi Module Control Program
 *  Copyright 2019 Norikazu Goto
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
/*-------------------------------------------------------------------------*
 * Globals:
 *-------------------------------------------------------------------------*/
TelitWiFi gs2200;
TWIFI_Params gsparams;

const uint8_t UDP_Data[] = "GS2200 UDP Client Data Transfer Test.";
const uint16_t UDP_PACKET_SIZE = 37; 

const uint16_t UDP_RECEIVE_PACKET_SIZE = 1500; 
uint8_t UDP_Receive_Data[UDP_RECEIVE_PACKET_SIZE] = {0};

/*-------------------------------------------------------------------------*
 * Function ProtoTypes:
 *-------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*
 * led_onoff
 *---------------------------------------------------------------------------*/
static void led_onoff(int num, bool stat)
{
	switch (num) {
	case 0:
		digitalWrite(LED0, stat);
		break;

	case 1:
		digitalWrite(LED1, stat);
		break;

	case 2:
		digitalWrite(LED2, stat);
		break;

	case 3:
		digitalWrite(LED3, stat);
		break;
	}

}

/*---------------------------------------------------------------------------*
 * led_effect
 *---------------------------------------------------------------------------*
 * Description: See this effect....
 *---------------------------------------------------------------------------*/
static void led_effect(void)
{
	static int cur=0;
	int i;
	static bool direction=true; // which way to go
	

	for (i=-1; i<5; i++) {
		if (i==cur) {
			led_onoff(i, true);
			if (direction)
				led_onoff(i-1, false);
			else
				led_onoff(i+1, false);
		}
	}

	if (direction) { // 0 -> 1 -> 2 -> 3
		if (++cur > 4)
			direction = false;
	}
	else {
		if (--cur < -1)
			direction = true;
	}
		
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

	/* Initialize AT Command Library Buffer */
	AtCmd_Init();
	/* Initialize SPI access of GS2200 */
	Init_GS2200_SPI_type(iS110B_TypeC);
	/* Initialize AT Command Library Buffer */
	gsparams.mode = ATCMD_MODE_STATION;
	gsparams.psave = ATCMD_PSAVE_DEFAULT;
	if (gs2200.begin(gsparams)) {
		ConsoleLog("GS2200 Initilization Fails");
		while(1);
	}

	/* GS2200 Association to AP */
	if (gs2200.activate_station(AP_SSID, PASSPHRASE)) {
		ConsoleLog("Association Fails");
		while(1);
	}
	digitalWrite(LED0, HIGH); // turn on LED

}

// the loop function runs over and over again forever
void loop() {
	char server_cid = 0;
	bool served = false;
	uint32_t timer = 0;
  int receive_size = 0;
	while (1) {
		if (!served) {
			ConsoleLog("Start UDP Client");
			// Create UDP Client
      server_cid = gs2200.connectUDP(UDPSRVR_IP, UDPSRVR_PORT, LocalPort);
			ConsolePrintf("server_cid: %d \r\n", server_cid);
			if (server_cid == ATCMD_INVALID_CID) {
				continue;
			}
			served = true;
		}
		else {
			ConsoleLog("Start to send UDP Data");
			// Prepare for the next chunck of incoming data
			WiFi_InitESCBuffer();
			ConsolePrintf("\r\n");

			while (1) {
        gs2200.write(server_cid, UDP_Data, UDP_PACKET_SIZE);

        // Description: Wait for a response after sending a command. Keep parsing the data until a response is found.
        receive_size = gs2200.read(server_cid, UDP_Receive_Data, UDP_RECEIVE_PACKET_SIZE);
        if (0 < receive_size) {
          ConsolePrintf("%d byte Recieved successfully. \r\n", receive_size);
          for (int i = 0; i < receive_size; i++) {
            ConsolePrintf("%c", UDP_Receive_Data[i]);
          }
          ConsolePrintf("\r\n");
          
          memset(UDP_Receive_Data, 0, UDP_RECEIVE_PACKET_SIZE);
          WiFi_InitESCBuffer();
          delay(100);
        }

        if (msDelta(timer) > 100) {
          timer = millis();
          led_effect();
        }
      }
    }
  }
}
