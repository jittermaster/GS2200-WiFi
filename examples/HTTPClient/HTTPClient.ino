/*
 *  HTTPClient.ino - GainSpan WiFi Module Control Program
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

#include <HttpGs2200.h>
#include <TelitWiFi.h>
#include "config.h"

#define  CONSOLE_BAUDRATE  115200

typedef enum{
	POST=0,
	GET
} DEMO_STATUS_E;

DEMO_STATUS_E httpStat;
char sendData[100];

const uint16_t RECEIVE_PACKET_SIZE = 1500;
uint8_t Receive_Data[RECEIVE_PACKET_SIZE] = {0};

TelitWiFi gs2200;
TWIFI_Params gsparams;
HttpGs2200 theHttpGs2200(&gs2200);
HTTPGS2200_HostParams hostParams;

int count = 0;

void parse_httpresponse(char *message)
{
	char *p;
	
	if ((p=strstr(message, "200 OK\r\n")) != NULL) {
		ConsolePrintf("Response : %s\r\n", p+8);
	}
}

void setup() {

	/* initialize digital pin LED_BUILTIN as an output. */
	pinMode(LED0, OUTPUT);
	digitalWrite(LED0, LOW);   // turn the LED off (LOW is the voltage level)
	Serial.begin(CONSOLE_BAUDRATE); // talk to PC

	/* Initialize SPI access of GS2200 */
	Init_GS2200_SPI_type(iS110B_TypeC);

	/* Initialize AT Command Library Buffer */
	gsparams.mode = ATCMD_MODE_STATION;
	gsparams.psave = ATCMD_PSAVE_DEFAULT;
	if (gs2200.begin(gsparams)) {
		ConsoleLog("GS2200 Initilization Fails");
		while (1);
	}

	/* GS2200 Association to AP */
	if (gs2200.activate_station(AP_SSID, PASSPHRASE)) {
		ConsoleLog("Association Fails");
		while (1);
	}

	hostParams.host = (char *)HTTP_SRVR_IP;
	hostParams.port = (char *)HTTP_PORT;
	theHttpGs2200.begin(&hostParams);

	ConsoleLog("Start HTTP Client");

	/* Set HTTP Headers */
	theHttpGs2200.config(HTTP_HEADER_AUTHORIZATION, "Basic dGVzdDp0ZXN0MTIz");
	theHttpGs2200.config(HTTP_HEADER_TRANSFER_ENCODING, "chunked");
	theHttpGs2200.config(HTTP_HEADER_CONTENT_TYPE, "application/x-www-form-urlencoded");
	theHttpGs2200.config(HTTP_HEADER_HOST, HTTP_SRVR_IP);

	digitalWrite(LED0, HIGH); // turn on LED

}

// the loop function runs over and over again forever
void loop() {
	httpStat = POST;
	bool result = false;

	while (1) {
		switch (httpStat) {
		case POST:
			theHttpGs2200.config(HTTP_HEADER_TRANSFER_ENCODING, "chunked");
			//create post data.
			snprintf(sendData, sizeof(sendData), "data=%d", count);
			result = theHttpGs2200.post("/postData", sendData);

			if (0 < theHttpGs2200.receive(Receive_Data, RECEIVE_PACKET_SIZE)) {
				parse_httpresponse( (char *)(Receive_Data) );
			} else {
				printf("theHttpGs2200.receive err.\n");
			}
			/* Need to receive the HTTP response */
			/* Timeout for 2000ms*/
			result = theHttpGs2200.receive(2000);
			result = theHttpGs2200.end();

			delay(1000);
			count+=100;
			httpStat = GET;
			break;
			
		case GET:
			theHttpGs2200.config(HTTP_HEADER_TRANSFER_ENCODING, "identity");

			result = theHttpGs2200.get(HTTP_PATH);
			if (true == result) {
				theHttpGs2200.get_data(Receive_Data, RECEIVE_PACKET_SIZE);
				parse_httpresponse((char *)(Receive_Data));
			} else {
				ConsoleLog( "?? Unexpected HTTP Response ??" );
			}
			result = theHttpGs2200.receive(2000);
			if (false == result) {
				theHttpGs2200.get_data(Receive_Data, RECEIVE_PACKET_SIZE);
				ConsolePrintf("%s", (char *)(Receive_Data));
			} else {
				// AT+HTTPSEND command is done
				ConsolePrintf( "\r\n");
			}

			result = theHttpGs2200.end();

			delay(1000);
			httpStat = POST;
			break;
		default:
			break;
		}
	}
}
