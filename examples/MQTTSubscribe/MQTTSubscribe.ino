/*
 *  SPRESENSE_WiFi.ino - GainSpan WiFi Module Control Program
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
#include "AppFunc.h"

extern uint8_t ESCBuffer[];
extern uint32_t ESCBufferCnt;

#define  CONSOLE_BAUDRATE  115200

#define  MQTT_SRVR     "test.mosquitto.org"
#define  MQTT_PORT     "1883"
#define  MQTT_CLI_ID   "Telit_Device_pub"
#define  MQTT_TOPIC    "Telit/property"


// the setup function runs once when you press reset or power the board
void setup() {

	/* initialize digital pin LED_BUILTIN as an output. */
	pinMode(LED0, OUTPUT);
	digitalWrite( LED0, LOW );   // turn the LED off (LOW is the voltage level)
	Serial.begin( CONSOLE_BAUDRATE ); // talk to PC

	/* Initialize SPI access of GS2200 */
	Init_GS2200_SPI();

	digitalWrite( LED0, HIGH ); // turn on LED

	/* Initialize AT Command Library Buffer */
	AtCmd_Init();

	/* GS2200 Association to AP */
	App_InitModule();
	App_ConnectAP();
}

char server_cid = 0;
bool served = false;
uint16_t len, count=0;
ATCMD_MQTTparams mqttparams;

// the loop function runs over and over again forever
void loop() {

	ATCMD_RESP_E resp;
	ATCMD_NetworkStatus networkStatus;

	if (!served) {
		resp = ATCMD_RESP_UNMATCH;
		// Start a MQTT client
		ConsoleLog( "Start MQTT Client");
    AtCmd_VER();
 
		resp = AtCmd_MQTTCONNECT( &server_cid, (char *)MQTT_SRVR, (char *)MQTT_PORT, (char *)MQTT_CLI_ID, NULL, NULL );
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

		do {
			resp = AtCmd_NSTAT(&networkStatus);
		} while (ATCMD_RESP_OK != resp);
		
		served = true;
	}
	else {
		ConsoleLog( "Start to send MQTT Message");
		// Prepare for the next chunck of incoming data
		WiFi_InitESCBuffer();
		
		// Start the loop to send the data
		strcpy( mqttparams.topic, MQTT_TOPIC );
		mqttparams.QoS = 0;
		mqttparams.retain = 0;
		if( ATCMD_RESP_OK == AtCmd_MQTTSUBSCRIBE( server_cid, mqttparams ) ){
			ConsolePrintf( "Subscribed! \n" );
		}
		while( served ) {

			/* just in case something from GS2200 */
			while( Get_GPIO37Status() ){

				String data;
				resp = AtCmd_RecieveMQTTData(data);

				if( ATCMD_RESP_DISCONNECT == resp ){
					served = false; // quite the loop
					break;
				}

				Serial.println("Recieve data: " + data);
			}
		}
	}
}
