/*
 *  TelitWiFi.cpp - Telit GS2000 Series WiFi Module Operations
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

#include "TelitWiFi.h"
#include <GS2200Hal.h>

extern uint8_t ESCBuffer[];
extern uint32_t ESCBufferCnt;

#define CMD_TIMEOUT 10000

// #define GS2200_DEBUG
#ifdef GS2200_DEBUG
#define gs2200_printf printf
#else
#define gs2200_printf(...) do {} while (0)
#endif

TelitWiFi::TelitWiFi()
{
}

TelitWiFi::~TelitWiFi()
{
}

/***
 * @brief Initialize GS2000 Module before associating to AP(Access Point)
 *        Set following parameters of AT commands
 *        AT+WM, AT+WRXACIVE
 * @param params - IN: WiFi parameters
 * @return 0: success, -1: failure
 *          
 */
int TelitWiFi::begin(TWIFI_Params params)
{
	ATCMD_RESP_E r = ATCMD_RESP_UNMATCH;
	ATCMD_REGDOMAIN_E regDomain;
	char macid[20];
	uint32_t start = millis();

	/* Try to read boot-up banner */
	while( Get_GPIO37Status() ){
		r = AtCmd_RecvResponse();
		if( r == ATCMD_RESP_NORMAL_BOOT_MSG )
			ConsoleLog("Normal Boot.\r\n");
	}

	while( 1 ){

		if( msDelta( start ) >= CMD_TIMEOUT )
			return FAIL;

		r = AtCmd_AT();
		if( ATCMD_RESP_OK != r ) continue;


		/* Send command to disable Echo */
		r = AtCmd_ATE(0);
		if( ATCMD_RESP_OK != r ) continue;

		/* AT+WREGDOMAIN=? should run after disabling Echo, otherwise the wrong domain is obtained. */
		r = AtCmd_WREGDOMAIN_Q( &regDomain );
		if( ATCMD_RESP_OK != r ) continue;

		/* If TELEC is not selected */
		if( regDomain != ATCMD_REGDOMAIN_TELEC ){
			r = AtCmd_WREGDOMAIN( ATCMD_REGDOMAIN_TELEC );
			if( ATCMD_RESP_OK != r ) continue;
		}

		/* Read MAC Address */
		r = AtCmd_NMAC_Q( macid );
		if( ATCMD_RESP_OK != r ) continue;

		/* Read Version Information */
		r = AtCmd_VER();
		if( ATCMD_RESP_OK != r ) continue;

		/* Enable Power save mode */
		/* AT+WRXACTIVE=0, AT+WRXPS=1 */
		r = AtCmd_WRXACTIVE( params.psave );
		if( ATCMD_RESP_OK != r ) continue;

		r = AtCmd_WRXPS(1);
		if( ATCMD_RESP_OK != r ) continue;

		/* Set Wireless mode */
		r = AtCmd_WM( params.mode );
		if( ATCMD_RESP_OK != r ) continue;

		if( ATCMD_MODE_LIMITED_AP==params.mode ){
			r = AtCmd_WSEC( ATCMD_SEC_WPA2PSK ); // WPA2-personal
			if( ATCMD_RESP_OK != r ) continue;

			/* Disable DHCP Client */
			r = AtCmd_NDHCP( 0 );
			if( ATCMD_RESP_OK != r ) continue;

			/* Disable DHCP Server */
			/* This is necessary, otherwise AT+DHCPSRVR=1 may return ERROR so that we need to run this loop again */
			r = AtCmd_DHCPSRVR( 0 );
			/* Enable DHCP Server */
			r = AtCmd_DHCPSRVR( 1 );
			if( ATCMD_RESP_OK != r ) continue;
			
		}
		else{
			/* Enable DHCP Client */
			r = AtCmd_NDHCP( 1 );
			if( ATCMD_RESP_OK != r ) continue;
		}
		
		/* Bulk Data mode */
		r = AtCmd_BDATA(1);
		if( ATCMD_RESP_OK != r ) continue;
	
		return OK;
	}
}

/**
 * @brief Association to AP in Station mode
 * @param const char *ssid - IN: AP SSID
 *        const char *passphrase - IN: WPA2 Passphrase
 * @return 0: success, -1: failure
 */
int TelitWiFi::activate_station(const String& ssid, const String& passphrase)
{
	ATCMD_RESP_E r;
	uint32_t start = millis();

	ConsoleLog("Associate to Access Point");

	while( 1 ){
		if( msDelta( start ) >= 2*CMD_TIMEOUT )
			return FAIL;

		/* Try to disassociate if not already associated */
		r = AtCmd_WD(); 
		if( ATCMD_RESP_OK != r ) continue;

		/* Enable DHCP Client */
		r = AtCmd_NDHCP( 1 );
		if( ATCMD_RESP_OK != r ) continue;

		/* Set WPA2 Passphrase */
		r = AtCmd_WPAPSK( String(ssid).c_str(), String(passphrase).c_str() );
		if( ATCMD_RESP_OK != r ) continue;

		/* Associate with AP */
		r = AtCmd_WA( String(ssid).c_str(), "", 0 );
		if( ATCMD_RESP_OK != r ) continue;

		return OK;
	}
}

/**
 * @brief Association to AP in Limited-AP mode
 * @param const char *ssid - IN: AP SSID
 *        const char *passphrase - IN: WPA2 Passphrase
 * @return 0: success, -1: failure
 */
int TelitWiFi::activate_ap(const String& ssid, const String& passphrase, uint8_t channel)
{
	ATCMD_RESP_E r;
	uint32_t start = millis();

	ConsoleLog("Establish Wireless Network");

	while( 1 ){
		if( msDelta( start ) >= 2*CMD_TIMEOUT )
			return FAIL;

		/* Try to disassociate if not already associated */
		r = AtCmd_WD(); 
		if( ATCMD_RESP_OK != r ) continue;

		/* Set WPA2 Passphrase */
		r = AtCmd_WPAPSK( String(ssid).c_str(), String(passphrase).c_str() );
		if( ATCMD_RESP_OK != r ) continue;

		/* Associate with AP */
		r = AtCmd_WA( String(ssid).c_str(), "", channel );
		if( ATCMD_RESP_OK != r ) continue;

		return OK;
	}
}

/**
 * @brief Connect TCP server
 * @param const char *ip - IN: IP
 *        const uint16_t *port - IN: Port
 * @return char cid: Channel ID
 */
char TelitWiFi::connect(const String& ip, const String& port)
{
	ATCMD_RESP_E resp;
	char cid = ATCMD_INVALID_CID;
	ATCMD_NetworkStatus networkStatus;

	resp = ATCMD_RESP_UNMATCH;
	ConsoleLog( "Start TCP Client");
	WiFi_InitESCBuffer();

	resp = AtCmd_NCTCP( String(ip).c_str() , String(port).c_str(), &cid);

	if (resp != ATCMD_RESP_OK) {
		ConsoleLog( "No Connect!" );
		delay(2000);
		return cid;
	}

	if (cid == ATCMD_INVALID_CID) {
		ConsoleLog( "No CID!" );
		delay(2000);
		return cid;
	}

	do {
		resp = AtCmd_NSTAT(&networkStatus);
	} while (ATCMD_RESP_OK != resp);

	ConsoleLog( "Connected" );
	ConsolePrintf("IP: %d.%d.%d.%d\r\n", 
	              networkStatus.addr.ipv4[0], networkStatus.addr.ipv4[1], networkStatus.addr.ipv4[2], networkStatus.addr.ipv4[3]);
	return cid;

}

/**
 * @brief Connect UDP server
 * @param const String& ip - IN: IP
 *        const String& port - IN: Port
 * 				const String& srcPort - IN: srcPort
 * @return char cid: Channel ID
 */
char TelitWiFi::connectUDP(const String& ip, const String& port, const String& srcPort)
{
	ATCMD_RESP_E resp;
	char cid = ATCMD_INVALID_CID;
	ATCMD_NetworkStatus networkStatus;

	resp = ATCMD_RESP_UNMATCH;
	ConsoleLog( "Start UDP Client");
	WiFi_InitESCBuffer();

	resp = AtCmd_NCUDP( String(ip).c_str(), String(port).c_str(), String(srcPort).c_str(), &cid);

	if (resp != ATCMD_RESP_OK) {
		ConsoleLog( "No Connect!" );
		delay(2000);
		return cid;
	}

	if (cid == ATCMD_INVALID_CID) {
		ConsoleLog( "No CID!" );
		delay(2000);
		return cid;
	}

	do {
		resp = AtCmd_NSTAT(&networkStatus);
	} while (ATCMD_RESP_OK != resp);

	ConsoleLog( "Connected" );
	ConsolePrintf("IP: %d.%d.%d.%d\r\n",
	              networkStatus.addr.ipv4[0], networkStatus.addr.ipv4[1], networkStatus.addr.ipv4[2], networkStatus.addr.ipv4[3]);
	return cid;

}

/**
 * @brief stop server
 * @param char cid: Channel ID
 * 
 */
void TelitWiFi::stop(char cid)
{
	ATCMD_RESP_E resp;

	while( !Get_GPIO37Status() );

	while( Get_GPIO37Status() ){
		resp = AtCmd_RecvResponse();

		if( ATCMD_RESP_BULK_DATA_RX == resp ){

			WiFi_InitESCBuffer();

		}else if( ATCMD_RESP_DISCONNECT == resp ){
			resp = AtCmd_NCLOSE( cid );
			resp = AtCmd_NCLOSEALL();
			WiFi_InitESCBuffer();
		}

		sleep(2);

	}

}

/**
 * @brief Is connected TCP server
 * @param char cid: Channel ID
 * 
 */
bool TelitWiFi::connected(char cid)
{
	if( ( cid < '0' ) && ( cid > 'f' ) ){
		return false;
	}
	return true;
}

/**
 * @brief Send data to TCP server
 * @param char cid: Channel ID
 *        const uint8_t *data - IN: deta pointer
 *        const int length - IN: data size
 * 
 */
bool TelitWiFi::write(char cid, const uint8_t* data, uint16_t length)
{
	if( ATCMD_RESP_OK != AtCmd_SendBulkData( cid, data, length )){
		// Data is not sent, we need to re-send the data
		gs2200_printf( "Send Error.");
		return false;
	}

	return true;

}

bool TelitWiFi::available()
{
	return Get_GPIO37Status();
}

int TelitWiFi::read(char cid, uint8_t* data, int length)
{
	int size = -1;
	ATCMD_RESP_E resp;

	resp = AtCmd_RecvResponse();

	if( ATCMD_RESP_BULK_DATA_RX == resp ){
		if( Check_CID( cid ) ){
			size = ESCBufferCnt-1;
			if(size > length){
				size = length;
				ConsoleLog( "Lost some data.");
			}
			memcpy(data,(ESCBuffer + 1),size);
		}else{
			ConsoleLog( "Missmatch cid.");
		}
	}else{
		gs2200_printf( "Recieve another event.");
	}

	return size;
}
