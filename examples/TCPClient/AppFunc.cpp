/*
 *  AppFunc.cpp - Application code of GS2200
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


/*-------------------------------------------------------------------------*
 * Includes:
 *-------------------------------------------------------------------------*/
#include <Arduino.h>
#include <GS2200AtCmd.h>
#include <GS2200Hal.h>
#include "AppFunc.h"
#include "config.h"


/*-------------------------------------------------------------------------*
 * Constants:
 *-------------------------------------------------------------------------*/

#define  APP_DEBUG


/*-------------------------------------------------------------------------*
 * Types:
 *-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*
 * Globals:
 *-------------------------------------------------------------------------*/
char TCP_Data[]="GS2200 TCP Client Data Transfer Test";

extern uint8_t ESCBuffer[];
extern uint32_t ESCBufferCnt;


/*-------------------------------------------------------------------------*
 * Locals:
 *-------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------*
 * Function ProtoTypes:
 *-------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*
 * led_onoff
 *---------------------------------------------------------------------------*/
static void led_onoff( int num, bool stat )
{
	switch( num ){
	case 0:
		digitalWrite( LED0, stat );
		break;

	case 1:
		digitalWrite( LED1, stat );
		break;

	case 2:
		digitalWrite( LED2, stat );
		break;

	case 3:
		digitalWrite( LED3, stat );
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
	static int cur=0, next;
	int i;
	static bool direction=true; // which way to go
	

	for( i=-1; i<5; i++ ){
		if( i==cur ){
			led_onoff( i, true );
			if( direction )
				led_onoff( i-1, false );
			else
				led_onoff( i+1, false );
		}
	}

	if( direction ){ // 0 -> 1 -> 2 -> 3
		if( ++cur > 4 )
			direction = false;
	}
	else {
		if( --cur < -1 )
			direction = true;
	}
		
}

/*---------------------------------------------------------------------------*
 * App_InitModule
 *---------------------------------------------------------------------------*
 * Description: Minimum set of node initialization AT commands
 *---------------------------------------------------------------------------*/
void App_InitModule(void)
{
	ATCMD_RESP_E r = ATCMD_RESP_UNMATCH;
	ATCMD_REGDOMAIN_E regDomain;
	char macid[20];

	/* Try to read boot-up banner */
	while( Get_GPIO37Status() ){
		r = AtCmd_RecvResponse();
		if( r == ATCMD_RESP_NORMAL_BOOT_MSG )
			ConsoleLog("Normal Boot.\r\n");
	}
	
	do {
		r = AtCmd_AT();
	} while (ATCMD_RESP_OK != r);


	/* Send command to disable Echo */
	do {
		r = AtCmd_ATE(0);
	} while (ATCMD_RESP_OK != r);

	/* AT+WREGDOMAIN=? should run after disabling Echo, otherwise the wrong domain is obtained. */
	do {
		r = AtCmd_WREGDOMAIN_Q( &regDomain );
	} while (ATCMD_RESP_OK != r);

	if( regDomain != ATCMD_REGDOMAIN_TELEC ){
		do {
			r = AtCmd_WREGDOMAIN( ATCMD_REGDOMAIN_TELEC );
		} while (ATCMD_RESP_OK != r);
	}	

	/* Read MAC Address */
	do{
		r = AtCmd_NMAC_Q( macid );
	}while(ATCMD_RESP_OK != r); 

	/* Read Version Information */
	do {
		r = AtCmd_VER();
	} while (ATCMD_RESP_OK != r);


	/* Enable Power save mode */
	/* AT+WRXACTIVE=0, AT+WRXPS=1 */
	do{
		r = AtCmd_WRXACTIVE(0);
	}while(ATCMD_RESP_OK != r); 

	do{
		r = AtCmd_WRXPS(1);
	}while(ATCMD_RESP_OK != r); 

	/* Bulk Data mode */
	do{
		r = AtCmd_BDATA(1);
	}while(ATCMD_RESP_OK != r); 

}


/*---------------------------------------------------------------------------*
 * App_ConnectAP
 *---------------------------------------------------------------------------*
 * Description: Associate to the AP
 *---------------------------------------------------------------------------*/
void App_ConnectAP(void)
{
	ATCMD_RESP_E r;

#ifdef APP_DEBUG
	ConsolePrintf("Associating to AP: %s\r\n", AP_SSID);
#endif

	/* Set Infrastructure mode */
	do { 
		r = AtCmd_WM(ATCMD_MODE_STATION);
	}while (ATCMD_RESP_OK != r);

	/* Try to disassociate if not already associated */
	do { 
		r = AtCmd_WD(); 
	}while (ATCMD_RESP_OK != r);

	/* Enable DHCP Client */
	do{
		r = AtCmd_NDHCP( 1 );
	}while(ATCMD_RESP_OK != r); 

	/* Set WPA2 Passphrase */
	do{
		r = AtCmd_WPAPSK( (char *)AP_SSID, (char *)PASSPHRASE );
	}while(ATCMD_RESP_OK != r); 

	/* Associate with AP */
	do{
		r = AtCmd_WA( (char *)AP_SSID, (char *)"", 0 );
	}while(ATCMD_RESP_OK != r); 

	/* L2 Network Status */
	do{
		r = AtCmd_WSTATUS();
	}while(ATCMD_RESP_OK != r); 

}


/*---------------------------------------------------------------------------*
 * App_TCPClient_Test
 *---------------------------------------------------------------------------*
 * Description: 
 *     Connect TCP server, send data on and on
 *     Show the received data from the TCP server
 *---------------------------------------------------------------------------*/

void App_TCPClient_Test(void)
{
	ATCMD_RESP_E resp;
	char server_cid = 0;
	bool served = false;
	ATCMD_NetworkStatus networkStatus;
	uint32_t timer=0;

	AtCmd_Init();

	App_InitModule();
	App_ConnectAP();

	while (1) {
		if (!served) {
			resp = ATCMD_RESP_UNMATCH;
			// Start a TCP client
			ConsoleLog( "Start TCP Client");
			resp = AtCmd_NCTCP( (char *)TCPSRVR_IP, (char *)TCPSRVR_PORT, &server_cid);
			if (resp != ATCMD_RESP_OK) {
				ConsoleLog( "No Connect!" );
				delay(2000);
				continue;
			}
			if (server_cid == ATCMD_INVALID_CID) {
				ConsoleLog( "No CID!" );
				delay(2000);
				continue;
			}
			
			do {
				resp = AtCmd_NSTAT(&networkStatus);
			} while (ATCMD_RESP_OK != resp);
			
			ConsoleLog( "Connected" );
			ConsolePrintf("IP: %d.%d.%d.%d\r\n", 
				      networkStatus.addr.ipv4[0], networkStatus.addr.ipv4[1], networkStatus.addr.ipv4[2], networkStatus.addr.ipv4[3]);

			served = true;
		}
		else {
			ConsoleLog( "Start to send TCP Data");
			// Prepare for the next chunck of incoming data
			WiFi_InitESCBuffer();

			// Start the infinite loop to send the data
			while( 1 ){
				if( ATCMD_RESP_OK != AtCmd_SendBulkData( server_cid, TCP_Data, strlen(TCP_Data) ) ){
					// Data is not sent, we need to re-send the data
					delay(10);
				}
				
				while( Get_GPIO37Status() ){
					resp = AtCmd_RecvResponse();

					if( ATCMD_RESP_BULK_DATA_RX == resp ){
						if( Check_CID( server_cid ) ){
							ConsolePrintf( "Receive %d byte:%s \r\n", ESCBufferCnt-1, ESCBuffer+1 );
						}
						
						WiFi_InitESCBuffer();
					}

				}

				if( msDelta( timer ) > 100 ){
					timer = millis();
					led_effect();
				}
			}

		}
	}

}


/*-------------------------------------------------------------------------*
 * End of File:  AppFunc.c
 *-------------------------------------------------------------------------*/
