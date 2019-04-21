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


#define CMD_TIMEOUT 10000



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
int TelitWiFi::connect(const char *ssid, const char *passphrase)
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

		/* Set WPA2 Passphrase */
		r = AtCmd_WPAPSK( (char *)ssid, (char *)passphrase );
		if( ATCMD_RESP_OK != r ) continue;

		/* Associate with AP */
		r = AtCmd_WA( (char *)ssid, (char *)"", 0 );
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
int TelitWiFi::connect(const char *ssid, const char *passphrase, uint8_t channel)
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
		r = AtCmd_WPAPSK( (char *)ssid, (char *)passphrase );
		if( ATCMD_RESP_OK != r ) continue;

		/* Associate with AP */
		r = AtCmd_WA( (char *)ssid, (char *)"", channel );
		if( ATCMD_RESP_OK != r ) continue;

		return OK;
	}
}
