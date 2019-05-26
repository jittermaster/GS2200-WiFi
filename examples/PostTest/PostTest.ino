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

#include <GS2200Hal.h>
#include <GS2200AtCmd.h>
#include <TelitWiFi.h>
#include "config.h"


#define  CONSOLE_BAUDRATE  115200
#define  SEND_SIZE  100000


extern uint8_t  *RespBuffer[];
extern int   RespBuffer_Index;
extern uint8_t ESCBuffer[];
extern uint32_t ESCBufferCnt;


char server_cid = 0;
char httpsrvr_ip[16];
char sendData[SEND_SIZE+1];

TelitWiFi gs2200;
TWIFI_Params gsparams;


void parse_httpresponse(char *message)
{
	char *p;
	
	if( (p=strstr( message, "200 OK\r\n" )) != NULL ){
		ConsolePrintf( "Response : %s\r\n", p+8 );
	}
}


void setup() {

	/* initialize digital pin LED_BUILTIN as an output. */
	pinMode(LED0, OUTPUT);
	digitalWrite( LED0, LOW );   // turn the LED off (LOW is the voltage level)
	Serial.begin( CONSOLE_BAUDRATE ); // talk to PC

	/* Initialize SPI access of GS2200 */
	Init_GS2200_SPI();

	/* Initialize AT Command Library Buffer */
	gsparams.mode = ATCMD_MODE_STATION;
	gsparams.psave = ATCMD_PSAVE_DEFAULT;
	if( gs2200.begin( gsparams ) ){
		ConsoleLog( "GS2200 Initilization Fails" );
		while(1);
	}

	/* GS2200 Association to AP */
	if( gs2200.connect( AP_SSID, PASSPHRASE ) ){
		ConsoleLog( "Association Fails" );
		while(1);
	}

	digitalWrite( LED0, HIGH ); // turn on LED

}


// the loop function runs over and over again forever
void loop() {

	ATCMD_RESP_E resp;
	int count;
	bool httpresponse=false;
	uint32_t start;
	uint32_t size;
	char size_string[10];
	
	ConsoleLog( "Start HTTP Client");

	/* Set HTTP Headers */
	AtCmd_HTTPCONF( HTTP_HEADER_AUTHORIZATION, "Basic dGVzdDp0ZXN0MTIz" );
	AtCmd_HTTPCONF( HTTP_HEADER_CONTENT_TYPE, "application/x-www-form-urlencoded" );
	AtCmd_HTTPCONF( HTTP_HEADER_HOST, HTTP_SRVR_IP );

	/* Prepare for the next chunck of incoming data */
	WiFi_InitESCBuffer();
	count = 0;

	size = SEND_SIZE;
	memset( sendData, '0', SEND_SIZE );
	sendData[SEND_SIZE] = 0;
	ConsoleLog( "POST Start" );

	do {
		resp = AtCmd_HTTPOPEN( &server_cid, HTTP_SRVR_IP, HTTP_PORT );
	} while (ATCMD_RESP_OK != resp);
	
	ConsoleLog( "Socket Opened" );
		
	while( 1 ){
		/* Content-Length should be set BEFORE sending the data */
		sprintf( size_string, "%d", size );
		do {
			resp = AtCmd_HTTPCONF( HTTP_HEADER_CONTENT_LENGTH, size_string );
		} while (ATCMD_RESP_OK != resp);
		
		do {
			resp = AtCmd_HTTPSEND( server_cid, HTTP_METHOD_POST, 10, "/postData", sendData, size );
		} while (ATCMD_RESP_OK != resp);
		
		/* Need to receive the HTTP response */
		while( 1 ){
			if( Get_GPIO37Status() ){
				resp = AtCmd_RecvResponse();
				
				if( ATCMD_RESP_BULK_DATA_RX == resp ){
					if( Check_CID( server_cid ) ){
						parse_httpresponse( (char *)(ESCBuffer+1) );
					}
				}
				WiFi_InitESCBuffer();
				break;
			}
		}
		
		start = millis();
		while(1){
			if( Get_GPIO37Status() ){
				resp = AtCmd_RecvResponse();
				if( ATCMD_RESP_OK == resp ){
					// AT+HTTPSEND command is done
					break;
				}
			}
			if( msDelta(start)>20000 ) // Timeout
				break;
		}
		
		delay( 1000 );

	}

	do {
		resp = AtCmd_HTTPCLOSE( server_cid );
	} while( (ATCMD_RESP_OK != resp) && (ATCMD_RESP_INVALID_CID != resp) );
	ConsoleLog( "Socket Closed" );
}
