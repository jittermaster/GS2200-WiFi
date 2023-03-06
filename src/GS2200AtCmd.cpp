/*
 *  AtCmd.cpp - GS2200 AT command and response handler
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
#include "GS2200AtCmd.h"
#include "GS2200Hal.h"


/*-------------------------------------------------------------------------*
 * Constants:
 *-------------------------------------------------------------------------*/
#define SPI_MAX_SIZE   1400
#define TXBUFFER_SIZE  SPI_MAX_SIZE
#define RXBUFFER_SIZE  1500

#define NUM_OF_RESPBUFFER  32
#define WS_MAXENTRIES      (NUM_OF_RESPBUFFER - 1)

//#define ATCMD_DEBUG_ENABLE


/*-------------------------------------------------------------------------*
 * Globals:
 *-------------------------------------------------------------------------*/

/* Transmit buffer to send <ESC> sequence data stream to GS2200 */
uint8_t  TxBuffer[TXBUFFER_SIZE];
/* Receive buffer to save data from GS2200 */
uint8_t  RxBuffer[RXBUFFER_SIZE];

/* Receive buffer array to save response/message from GS2200 */
uint8_t  *RespBuffer[NUM_OF_RESPBUFFER];
int   RespBuffer_Index=0;



/*-------------------------------------------------------------------------*
 * Function Prototypes:
 *-------------------------------------------------------------------------*/
static void ConvertNumberTo4DigitASCII(uint16_t num, char *str);
static ATCMD_SECURITYMODE_E ParseSecurityMode(const char *string);
static void AtCmd_ParseIPAddress(const char *string, ATCMD_IP *ip);
static uint8_t ParseIntoTokens(char *line, char deliminator, char *tokens[], uint8_t maxTokens);
static char Search_CID( uint8_t *string );


/*-------------------------------------------------------------------------*
 * 
 * Tool Functions
 * 
 *-------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*
 * ConvertNumberTo4DigitASCII
 *---------------------------------------------------------------------------*
 * Description: Convert a number to 4 digit ASCII string
 * Inputs: uint16_t num -- number for he conversion
 *         char *str -- 4 digit ASCII returned back
 *---------------------------------------------------------------------------*/
static void ConvertNumberTo4DigitASCII(uint16_t num, char *str)
{
	/* num must be less than 10000 */
	str[0] = num/1000 + 0x30;
	str[1] = (num/100)%10 + 0x30;
	str[2] = (num/10)%10 + 0x30;
	str[3] = num%10 + 0x30;
	str[4] = '\0';
}

/*---------------------------------------------------------------------------*
 * ParseSecurityMode
 *---------------------------------------------------------------------------*
 * Description: Convert a security mode string into a security enumerated type.
 * Inputs: const char *string -- String with security mode
 *---------------------------------------------------------------------------*/
static ATCMD_SECURITYMODE_E ParseSecurityMode(const char *string)
{
	if (strcmp(string, "WPA2-PERSONAL") == 0) {
		return ATCMD_SEC_WPA2PSK;
	} else if (strcmp(string, "WPA-PERSONAL") == 0) {
		return ATCMD_SEC_WPAPSK;
	} else if (strcmp(string, "WPA-ENTERPRISE") == 0) {
		return ATCMD_SEC_WPA_E;
	} else if (strcmp(string, "WPA2-ENTERPRISE") == 0) {
		return ATCMD_SEC_WPA2_E;
	} else if (strncmp(string, "WEP", 3) == 0) {
		return ATCMD_SEC_WEP;
	} else if (strcmp(string, "NONE") == 0) {
		return ATCMD_SEC_OPEN;
	}
	return ATCMD_SEC_UNKNOWN;
}

/*---------------------------------------------------------------------------*
 * ParseIntoTokens
 *---------------------------------------------------------------------------*
 * Description: Converts a line into a list of tokens. Skips spaces before a token starts.
 * Inputs: char *line -- Pointer to an array of line
 *         char deliminator -- Character that separates the fields
 *         char *tokens[] -- Array of token pointer
 *         uint8_t maTokens -- max number of token
 *---------------------------------------------------------------------------*/
static uint8_t ParseIntoTokens(char *line, char deliminator, char *tokens[], uint8_t maxTokens)
{
	char *p = line;
	char c;
	int mode = 0;
	uint8_t numTokens = 0;
	char *lastNonWhitespace;
	
	/* Walk through all the characters and determine where the tokens are */
	/* while placing end of token characters on each token */
	while (numTokens < maxTokens) {
		c = *p;
		switch (mode) {
		case 0: /* looking for start */
			tokens[numTokens] = p;
			mode = 1;
			/* Fall into looking for a non-white space character immediately */
		case 1:
			/* Skip any white space at the beginning (by staying in this state) */
			if (isspace(c))
				break;
			tokens[numTokens] = p;
			lastNonWhitespace = p;
			mode = 2;
			/* Fall into mode 2 if not a white space and process immediately */
		case 2:
			/* Did we find the end of the token? */
			if ((c == deliminator) || (c == '\0')) {
				/* Null terminate the token after the last non-whitespace and */
				/* go back to finding the next token */
				lastNonWhitespace[1] = '\0';
				numTokens++;
				mode = 0;
			} else {
				/* looking at normal characters here */
				if (!isspace(c))
					lastNonWhitespace = p;
			}
			break;
		}
		/* Reached end of string */
		if (c == '\0')
			break;
		p++;
	}
	
	return numTokens;
}

/*---------------------------------------------------------------------------*
 * AtCmd_ParseIPAddress
 *---------------------------------------------------------------------------*
 * Description: Take the given string and parse into ip numbers
 * Inputs: const char *string -- String with IP number
 *         ATCMD_IP *ip -- Structure to receive parsed IP number
 *---------------------------------------------------------------------------*/
static void AtCmd_ParseIPAddress(const char *string, ATCMD_IP *ip)
{
	int v1, v2, v3, v4;

	/* Currently only parses ipv4 addresses */
	sscanf(string, "%d.%d.%d.%d", &v1, &v2, &v3, &v4);
	ip->ipv4[0] = v1;
	ip->ipv4[1] = v2;
	ip->ipv4[2] = v3;
	ip->ipv4[3] = v4;
}

/*---------------------------------------------------------------------------*
 * AtCmd_ParseIPAddress
 *---------------------------------------------------------------------------*
 * Description: Take the given string and parse into ip numbers
 * Inputs: const char *string -- String with IP number
 *         ATCMD_IP *ip -- Structure to receive parsed IP number
 *---------------------------------------------------------------------------*/
static char Search_CID( uint8_t *string )
{
	int i, len;

	len = strlen( (char *)string );
	for( i=0; i<len; i++ ){
		if( (string[i]>='0' && string[i]<='9') || (string[i]>='a' && string[i]<='f') )
			return (char)string[i];
	}

	return ATCMD_INVALID_CID;

}



/*
 *---------------------------<AT command list >--------------------------------------------------------------------------
 *
 _________________________________________________________________________________________________________________________
 AT Command                                                                   Description and AT command library API Name
 _________________________________________________________________________________________________________________________

 AT                                                                           Check the interface

 AT+VER=??                                                                    Get the Version Info

 ATE<0|1>                                                                     Disable/enable echo

 AT+NMAC=?                                                                    Get MAC address

 AT+WRXACTIVE=<0|1>                                                           Enable/disable the radio

 AT+WRXPS=<0|1>                                                               Enable/disable 802.11 power save

 AT+BDATA=<0/1>                                                               Bulk data reception enable/disable

 AT+WREGDOMAIN=?                                                              Output the current regulatory domain

 AT+WREGDOMAIN=<regDomain>                                                    Set the regulatory domain

 AT+WM=<0|2>                                                                  Set mode to Station (0) or Limited-AP (2)

 AT+WSEC=<n>                                                                  configure the different security configuration

 AT+WPAPSK=<SSID>,<PassPhrase>                                                Calculate and store the PSK

 AT+WA=<SSID>[[,<BSSID>][,<Ch>]]                                              Associate to AP or create the network

 AT+WWPS=<1/2>,<PIN>                                                          Associate to an AP using WPS.
   1 - Push Button mathod.
   2 - PIN mathod.

 AT+WS[=<SSID>[,<BSSID>][,<Ch>][,<ScanTime>]]                                 Perform wireless scan

 AT+WSTATUS                                                                   Display current Wireless Status

 AT+WD                                                                        Disassociate from current network

 AT+NDHCP=<0|1>                                                               Disable/Enable DHCP Client

 AT+DHCPSRVR=<0|1>                                                            Start/Stop DHCP Server

 AT+NSET=<IP>,<NetMask>,<Gateway>                                             Configure network address

 AT+NSTAT=?                                                                   Display current network context

 AT+CID=?                                                                     Display The CID info

 AT+NCTCP=<IP>,<Port>                                                         Open TCP client

 AT+NCUDP=<IP>,<RemotePort>,[<LocalPort>                                      Open UDP client

 AT+NSTCP=<Port>                                                              Open TCP server on Port

 AT+NSUDP=<Port>                                                              Open UDP server on Port

 AT+NCLOSE=cid                                                                Close the specified connection

 AT+NCLOSEALL                                                                 Close all connections

 AT+NXSETSOCKOPT                                                              Configure a socket

 AT+PSDPSLEEP                                                                 Enable deep sleep

 AT+PSSTBY=<n>[,<delay time>,<alarm1-pol>,<alarm2-pol>]                       Standby request for n milliseconds

 AT+STORENWCONN                                                               Store the network context

 AT+RESTORENWCONN                                                             Restore the network context


 AT+PING=<Ip>,<Trails>,<Interval>,<Len>,<TOS>,<TTL>,<PAYLAOD(16 Bytes)>       Starts Ping
 _________________________________________________________________________________________________________________________*/



/*---------------------------------------------------------------------------*
 * AtCmd_Init
 *---------------------------------------------------------------------------*
 * Description: Initialize buffer
 *---------------------------------------------------------------------------*/
void AtCmd_Init(void)
{
	/* Flush the receive buffer */
	memset( TxBuffer, 0, TXBUFFER_SIZE );
	memset( RxBuffer, 0, RXBUFFER_SIZE );
	memset( RespBuffer[0], NULL, NUM_OF_RESPBUFFER );
}



/*------------------------------------  General Operations  ------------------------------------------*/

/*---------------------------------------------------------------------------*
 * AtCmd_AT
 *---------------------------------------------------------------------------*
 * Description: Check the interface is working
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_AT(void)
{
	return AtCmd_SendCommand( (char *)"AT\r\n" );
}

/*---------------------------------------------------------------------------*
 * AtCmd_VER
 *---------------------------------------------------------------------------*
 * Description: Get more detail S2W version
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_VER(void)
{
	return AtCmd_SendCommand( (char *)"AT+VER=??\r\n");
}

/*---------------------------------------------------------------------------*
 * AtCmd_ATE
 *---------------------------------------------------------------------------*
 * Description: Enable or disable the echo back to the host
 * Inputs: uint8_t mode -- 1: enable, 0: disable
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_ATE(uint8_t n)
{
	char cmd[8];

	sprintf(cmd, "ATE%d\r\n", n);

	return AtCmd_SendCommand(cmd);
}

/*---------------------------------------------------------------------------*
 * Routine:  AtCmd_RESET
 *---------------------------------------------------------------------------*
 * Description: Resets the whole system including the RTC domain.
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_RESET(void)
{
	return AtCmd_SendCommand( (char *)"AT+RESET=1\r\n");
}


/*-----------------------------------  Layer 2 (WiFi) Operations  --------------------------------*/

/*---------------------------------------------------------------------------*
 * AtCmd_NMAC_Q
 *---------------------------------------------------------------------------*
 * Description: Output the current MAC address of GS2200
 * Inputs: char *mac -- MAC addres if found
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_NMAC_Q(char *mac)
{
	ATCMD_RESP_E resp;
	
	resp = AtCmd_SendCommand( (char *)"AT+NMAC=?\r\n");

	if ((resp == ATCMD_RESP_OK) && RespBuffer_Index )
		strcpy(mac, (const char*)RespBuffer[0]);
	
	return resp;
}

/*---------------------------------------------------------------------------*
 * AtCmd_WRXACTIVE
 *---------------------------------------------------------------------------*
 * Description: Enable or disable the radio.
 * Inputs: uint8_t n -- 0: receiver off, 1: receiver on
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_WRXACTIVE(uint8_t n)
{
	char cmd[30];

	sprintf(cmd, "AT+WRXACTIVE=%d\r\n", n);

	return AtCmd_SendCommand(cmd);
}

/*---------------------------------------------------------------------------*
 * AtCmd_WRXPS
 *---------------------------------------------------------------------------*
 * Description: Enter power save mode.
 * Inputs: uint8_t n -- 0: disabe power save mode, 1: enable
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_WRXPS(uint8_t n)
{
	char cmd[20];

	sprintf(cmd, "AT+WRXPS=%d\r\n", n);

	return AtCmd_SendCommand(cmd);
}

/*---------------------------------------------------------------------------*
 * AtCmd_BData
 *---------------------------------------------------------------------------*
 * Description: Enable or disable bulk mode data transfer
 * Inputs: uint8_t n -- 0: disable bulk mode, 1: enable.
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_BDATA(uint8_t n)
{
	char cmd[20];

	sprintf(cmd, "AT+BDATA=%d\r\n", n);

	return AtCmd_SendCommand(cmd);
}

/*---------------------------------------------------------------------------*
 * AtCmd_WREGDOMAIN_Q
 *---------------------------------------------------------------------------*
 * Description: Gets the current regulatory domain in use.
 * Inputs: ATCMD_REGDOMAIN_E *regDomain -- Regulatory domain in use
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_WREGDOMAIN_Q(ATCMD_REGDOMAIN_E *regDomain)
{
	ATCMD_RESP_E resp;
	
	*regDomain = ATCMD_REGDOMAIN_UNKNOWN;

	resp = AtCmd_SendCommand( (char *)"AT+WREGDOMAIN=?\r\n");

	if( resp == ATCMD_RESP_OK ){
		if( !strncmp( (const char*)RespBuffer[0], "REG_DOMAIN=FCC", 14 ) )
			*regDomain = ATCMD_REGDOMAIN_FCC;
		else if( !strncmp( (const char*)RespBuffer[0], "REG_DOMAIN=ETSI", 15 ) )
			*regDomain = ATCMD_REGDOMAIN_ETSI;
		else if( !strncmp( (const char*)RespBuffer[0], "REG_DOMAIN=TELEC", 16) )
			*regDomain = ATCMD_REGDOMAIN_TELEC;
	}
	
	return resp;
}

/*---------------------------------------------------------------------------*
 * AtCmd_WREGDOMAIN
 *---------------------------------------------------------------------------*
 * Description: Sets the regulatory domain in use.
 * Inputs: ATCMD_REGDOMAIN_E *regDomain -- Regulatory domain to use
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_WREGDOMAIN(ATCMD_REGDOMAIN_E regDomain)
{
	char cmd[20];
	sprintf(cmd, "AT+WREGDOMAIN=%d\r\n", regDomain);
	return AtCmd_SendCommand(cmd);
}

/*---------------------------------------------------------------------------*
 * Routine:  AtCmd_Mode
 *---------------------------------------------------------------------------*
 * Description: Set the wireless mode
 * Inputs: uint8_t  mode -- wireless mode
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_WM(ATCMD_MODE_E mode)
{
	char cmd[20];

	sprintf(cmd, "AT+WM=%d\r\n", (uint8_t)mode);

	return AtCmd_SendCommand(cmd);
}

/*---------------------------------------------------------------------------*
 * AtCmd_WSEC
 *---------------------------------------------------------------------------*
 * Description: Configure the different security configuration
 *     AT+WSEC=n
 *                  0       Auto
 *                  1       Open
 *                  2       WEP
 *                  4       WPA
 *                  8       WPA2
 *                  16      WPA  - Enterprise
 *                  32      WPA2 - Enterprise
 *                  64      WPA2 - AES +TKIP security
 * Inputs:
 *      Security mode (see ATCMD_SECURITYMODE_E for values)
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_WSEC(ATCMD_SECURITYMODE_E security)
{
	char cmd[15];

	sprintf(cmd, "AT+WSEC=%d\r\n", security);
	return AtCmd_SendCommand(cmd);
}

/*---------------------------------------------------------------------------*
 * AtCmd_WPAPSK
 *---------------------------------------------------------------------------*
 * Description: Calculate and store the WPA2 PSK
 * Inputs: char *pSsid -- SSID (1 to 32 characters)
 *         char *pPsk -- Passphrase (8 to 63 characters)
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_WPAPSK(const char *pSsid, const char *pPsk)
{
	char cmd[60];
	
	sprintf(cmd, "AT+WPAPSK=%s,%s\r\n", pSsid, pPsk);
	
	return AtCmd_SendCommand(cmd);
}

/*---------------------------------------------------------------------------*
 * AtCmd_WA
 *---------------------------------------------------------------------------*
 * Description: Create or join and infrastructure network
 * Inputs: char *pSsid -- SSID to connect to (1 to 32 characters)
 *         char *pBssid -- BSSID of AP
 *         char channel -- Channel of network, 0 for any
 * Outputs:
 *      ATCMD_RESP_E -- error code
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_WA(const char *pSsid, const char *pBssid, uint8_t channel)
{
	char cmd[100];

	if (channel) {
		sprintf(cmd, "AT+WA=%s,%s,%d\r\n", pSsid, (pBssid) ? pBssid : "", channel);
	}
	else {
		sprintf(cmd, "AT+WA=%s\r\n", pSsid);
	}
	
	return AtCmd_SendCommand(cmd);
}

/*---------------------------------------------------------------------------*
 * AtCmd_WWPS
 *---------------------------------------------------------------------------*
 * Description: Associate to an AP using Wi-Fi Protected Setup (WPS)
 * Inputs: AtCmd_WPSResult *result -- WPS Result, if any
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_WWPS(uint8_t method, char *pin, ATCMD_WPSResult *result)
{
	ATCMD_RESP_E resp;
	int i;
	char cmd[30];
	
	if( method == 2 ){
		sprintf(cmd, "AT+WWPS=2,%s", pin);
		resp = AtCmd_SendCommand(cmd);
	}
	else
		resp = AtCmd_SendCommand( (char *)"AT+WWPS=1\r\n");

	/* wait for valid responce then parse the ssid, channel, and passphrase */
	if( resp == ATCMD_RESP_OK && RespBuffer_Index ){
		for( i=0; i<RespBuffer_Index; i++ ){
			if( NULL != strstr( (const char*)RespBuffer[i], "SSID=" ) ) {
				strcpy( result->ssid, (const char*)(RespBuffer[i]+5) );
			}
			else if( NULL != strstr( (const char*)RespBuffer[i], "CHANNEL=" ) ) {
				strcpy(result->ssid, (const char*)(RespBuffer[i]+8) );
			}
			else if( NULL != strstr( (const char*)RespBuffer[i], "PASSPHRASE=" ) ) {
				strcpy(result->ssid, (const char*)(RespBuffer[i]+11) );
			}
		}
	}

	return resp;
}


/*---------------------------------------------------------------------------*
 * AtCmd_WSTATUS
 *---------------------------------------------------------------------------*
 * Description: Retrieve information about the current wireless status
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_WSTATUS(void)
{
	ATCMD_RESP_E resp=ATCMD_RESP_UNMATCH;
	int i;
	
	resp = AtCmd_SendCommand( (char *)"AT+WSTATUS\r\n");
	if( resp == ATCMD_RESP_OK ){

		for( i=0; i<RespBuffer_Index-1; i++) {
			ConsolePrintf( "%s\n", RespBuffer[i] );
		}
	}
	return resp;
}

/*---------------------------------------------------------------------------*
 * AtCmd_WD
 *---------------------------------------------------------------------------*
 * Description: disassociate the current infrastructure network or stop the limited AP
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_WD(void)
{
	return AtCmd_SendCommand( (char *)"AT+WD\r\n");
}


/*--------------------------------  Layer 3/4 Operations  -------------------------------------*/

/*---------------------------------------------------------------------------*
 * AtCmd_NDHCP
 *---------------------------------------------------------------------------*
 * Description: Enable or disable DHCP Client.
 * Inputs: uint8_t mode -- 0: disable DHCP Client, 1: enable DHCP Client
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_NDHCP(uint8_t n)
{
	char cmd[15];

	sprintf(cmd, "AT+NDHCP=%d\r\n", n);
	
	return AtCmd_SendCommand(cmd);
}

/*---------------------------------------------------------------------------*
 * AtCmd_DHCPSRVR
 *---------------------------------------------------------------------------*
 * Description: EnabStart/Stop the DHCP server
 * Inputs: uint8_t start -- 1: Start DHCP server, 0: Stop DHCP server
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_DHCPSRVR(uint8_t start)
{
	char cmd[20];

	sprintf(cmd, "AT+DHCPSRVR=%d\r\n", start);
	return AtCmd_SendCommand(cmd);
}

/*---------------------------------------------------------------------------*
 * Routine:  AtCmd_NSET
 *---------------------------------------------------------------------------*
 * Description: Set network paremeters statically
 * Inputs: char *device -- IP Address string
 *         char *subnet -- Subnet mask string
 *         char *gateway -- Gateway string
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_NSET(char *device, char *subnet, char *gateway)
{
	char cmd[60];

	sprintf(cmd, "AT+NSET=%s,%s,%s\r\n", device, subnet, gateway);

	return AtCmd_SendCommand(cmd);
}

/*---------------------------------------------------------------------------*
 * AtCmd_NSTAT
 *---------------------------------------------------------------------------*
 * Description: Retrieve information about the current network status
 * Inputs: ATCMD_NetworkStatus *pStatus -- Pointer to structure status to fill.
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_NSTAT(ATCMD_NetworkStatus *pStatus)
{
	ATCMD_RESP_E resp;
	char *tokens[10];
	uint8_t numTokens;
	char *values[2];
	uint8_t numValues;
	uint8_t i, t;
	uint8_t rx = 0;

	memset(pStatus, 0, sizeof(*pStatus));

	resp = AtCmd_SendCommand( (char *)"AT+NSTAT=?\r\n");

	if( resp == ATCMD_RESP_OK ){
		for( i=0; i<RespBuffer_Index; i++) {
			numTokens = ParseIntoTokens( (char *)RespBuffer[i], ' ', tokens, 10);
			for( t=0; t<numTokens; t++) {
				numValues = ParseIntoTokens(tokens[t], '=', values, 2);
				if (numValues == 2) {
					if( strcmp(values[0], "MAC") == 0 ) {
						strcpy(pStatus->mac, values[1]);
					}
					else if( strcmp(tokens[t], "WSTATE") == 0) {
					       if( strcmp(values[1], "CONNECTED") == 0)
						       pStatus->connected = 1;
					}
					else if(strcmp(values[0], "BSSID") == 0) {
						strcpy(pStatus->bssid, values[1]);
					}
					else if(strcmp(values[0], "SSID") == 0) {
						strncpy(pStatus->ssid, values[1] + 1, strlen(values[1])- 2);
					}
					else if(strcmp(values[0], "CHANNEL") == 0) {
						pStatus->channel = atoi(values[1]);
					}
					else if(strcmp(values[0], "RSSI") == 0) {
						pStatus->signal = atoi(values[1]);
					}
					else if(strcmp(values[0], "SECURITY") == 0) {
						pStatus->security = ParseSecurityMode(values[1]);
					}
					else if(strcmp(values[0], /* IP */"addr") == 0) {
						AtCmd_ParseIPAddress(values[1], &pStatus->addr);
					}
					else if(strcmp(values[0], "SubNet") == 0) {
						AtCmd_ParseIPAddress(values[1], &pStatus->subnet);
					}
					else if(strcmp(values[0], "Gateway") == 0) {
						AtCmd_ParseIPAddress(values[1], &pStatus->gateway);
					}
					else if(strcmp(values[0], "DNS1") == 0) {
						AtCmd_ParseIPAddress(values[1], &pStatus->dns1);
					}
					else if(strcmp(values[0], "DNS2") == 0) {
						AtCmd_ParseIPAddress(values[1], &pStatus->dns2);
					}
					else if(strcmp(values[0], "Count") == 0) {
						if (rx) {
							pStatus->rxCount = atoi(values[1]);
						} else {
							pStatus->txCount = atoi(values[1]);
						}
					}
				} else if (numValues == 1) {
					if (strcmp(values[0], "Rx") == 0) {
						rx = 1;
					} else if (strcmp(values[0], "Tx") == 0) {
						rx = 0;
					}
				}
			}
		}
	}
	
	return resp;
}


/*---------------------------------------------------------------------------*
 * AtCmd_NCTCP
 *---------------------------------------------------------------------------*
 * Description: Create a TCP client connection
 * Inputs: char *desAddress -- server IP address string in format 12:34:56:78
 *         char *port -- Port string
 *         uint8_t *cid -- Client CID if connection is established
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_NCTCP( const char *destAddress, const char *port, char *cid)
{
	char cmd[40];
	ATCMD_RESP_E resp;
	char *result=NULL;
	
	sprintf(cmd, "AT+NCTCP=%s,%s\r\n", destAddress, port );

	resp = AtCmd_SendCommand(cmd);
	if( resp == ATCMD_RESP_OK && RespBuffer_Index ) {
		if( (result = strstr( (const char*)RespBuffer[0], "CONNECT")) != NULL) {
			/* Succesfull connection done for TCP client */
			*cid = result[8];
		}
		else{
			if( strstr( (const char*)RespBuffer[0], "IP" ) != NULL && 
			    (result = strstr( (const char*)RespBuffer[1], "CONNECT" ) ) != NULL ){
				/* Maybe destAddress is URL.
				   Need to check the second line of the response */
				/* Succesfull connection done for TCP client */
				*cid = result[8];
			}
			else{
				/* Not able to extract the CID */
				*cid = ATCMD_INVALID_CID;
				resp = ATCMD_RESP_ERROR;
			}
		}
	}
	
	return resp;
}



/*---------------------------------------------------------------------------*
 * AtCmd_NCUDP
 *---------------------------------------------------------------------------*
 * Description: Create a UDP client connection
 * Inputs: char *destAddress -- UDP server IP address string in format 12:34:56:78
 *         char *port -- Server port string
 *         char *srcPort -- Client port string
 *         uint8_t *cid -- Client CID
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_NCUDP(const char *destAddress, const char *port, const char *srcPort, char *cid )
{
	char cmd[60];
	ATCMD_RESP_E  resp;
	char *result = NULL;

	if( srcPort==NULL )
		sprintf(cmd, "AT+NCUDP=%s,%s\r\n", destAddress, port );
	else
		sprintf(cmd, "AT+NCUDP=%s,%s,%s\r\n", destAddress, port, srcPort );
	
	resp = AtCmd_SendCommand(cmd);
	if( resp == ATCMD_RESP_OK && RespBuffer_Index ){
		if( (result = strstr( (const char*)RespBuffer[0], "CONNECT" )) != NULL) {
			*cid = result[8];
		}
		else {
			/* Failed  */
			return ATCMD_RESP_ERROR;
		}
	}

	return resp;
}

/*---------------------------------------------------------------------------*
 * AtCmd_NSTCP
 *---------------------------------------------------------------------------*
 * Description:  Start the TCP server connection
 * Inputs: char *port -- Server port
 *         uint8_t *cid -- Server CID
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_NSTCP(char *port, char *cid)
{
	char cmd[20];
	ATCMD_RESP_E resp;
	char *result = NULL;
	
	sprintf(cmd, "AT+NSTCP=%s\r\n", port);
	
	resp = AtCmd_SendCommand(cmd);
	
	if (resp == ATCMD_RESP_OK && RespBuffer_Index ){
		if( (result = strstr((const char *)RespBuffer[0], "CONNECT")) != NULL) {
			*cid = result[8];
		} else {
			/* Failed  */
			return ATCMD_RESP_ERROR;
		}
	}
	return resp;
}


/*---------------------------------------------------------------------------*
 * AtCmd_NSUDP
 *---------------------------------------------------------------------------*
 * Description: Start a UDP server connection
 * Inputs: char *port -- Server Port
 *         uint8_t *cid -- Server CID 
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_NSUDP(char *port, char *cid)
{
	char cmd[20];
	char * result = NULL;
	ATCMD_RESP_E resp;
	
	sprintf(cmd, "AT+NSUDP=%s\r\n", port);
	
	resp = AtCmd_SendCommand(cmd);
	if( resp == ATCMD_RESP_OK && RespBuffer_Index ){
		if( (result = strstr((const char *)RespBuffer[0], "CONNECT")) != NULL) {
			*cid = result[8];
		} else {
			/* Failed  */
			return ATCMD_RESP_ERROR;
		}
	}

	return resp;
}

/*---------------------------------------------------------------------------*
 * AtCmd_NCLOSE
 *---------------------------------------------------------------------------*
 * Description: Close the connection of specified CID
 * Inputs: uint8_t cid -- CID
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_NCLOSE(uint8_t cid)
{
	char cmd[15];
	
	sprintf(cmd, "AT+NCLOSE=%c\r\n", cid);
	
	return AtCmd_SendCommand(cmd);
}

/*---------------------------------------------------------------------------*
 * AtCmd_NCLOSEALL
 *---------------------------------------------------------------------------*
 * Description: Close all open connections.
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_NCLOSEALL(void)
{
	return AtCmd_SendCommand( (char *)"AT+NCLOSEALL\r\n");
}

/*------------------------------------  Power Save Operations  ----------------------------------------*/

/*---------------------------------------------------------------------------*
 * AtCmd_PSDPSLEEP
 *---------------------------------------------------------------------------*
 * Description: Enable deep sleep.
 * Inputs: uint32_t x -- Standby time in milliseconds
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_PSDPSLEEP(uint32_t timeout)
{
	char cmd[30];

	if( timeout ){
		sprintf(cmd, "AT+PSDPSLEEP=%ld\r\n", timeout);
/*
  Note:
  GS2200 returns the response when timer expired.
  So, the host will wait for "timeout" period.
  This is the blocking task. If you don't like, please re-write this routine.
 */
		WiFi_Write( cmd, strlen(cmd) );

		while( !Get_GPIO37Status() );
	
		return AtCmd_RecvResponse();
	}
	else{
		sprintf(cmd, "AT+PSDPSLEEP\r\n");
		WiFi_Write( cmd, strlen(cmd) );

		return ATCMD_RESP_UNMATCH;
	}
}

/*---------------------------------------------------------------------------*
 * AtCmd_PSSTBY
 *---------------------------------------------------------------------------*
 * Description: Request a transistion to ultra-low-power Standby operation
 * Inputs: uint32_t x -- Standby time in milliseconds
 *         uint32_t delay -- Delay time in milliseconds before going into standby.
 *         uint8_t alarm1_pol -- polarity of RTC_IO_1 triggering an alarm
 *         uint8_t alarm2_pol -- polarity of RTC_IO_2 triggering an alarm
 * Note: GS2200 has only RTC_IO_2
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_PSSTBY(uint32_t x, uint32_t delay, uint8_t alarm1_pol, uint8_t alarm2_pol)
{
	char cmd[80];

	sprintf(cmd, "AT+PSSTBY=%ld,%ld,%d,%d\r\n", x, delay, alarm1_pol, alarm2_pol );
/*
  Note:
  GS2200 returns the response when exiting from the Standby mode.
  So, the host will wait till GS2200 wakes up
  This is the blocking task. If you don't like, please re-write this routine.
 */
	WiFi_Write( cmd, strlen(cmd) );

	while( !Get_GPIO37Status() );
	
	return AtCmd_RecvResponse();

}

/*---------------------------------------------------------------------------*
 * AtCmd_STORENWCONN
 *---------------------------------------------------------------------------*
 * Description: Store the network context.
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_STORENWCONN(void)
{
	return AtCmd_SendCommand( (char *)"AT+STORENWCONN\r\n");
}

/*---------------------------------------------------------------------------*
 * AtCmd_RESTORENWCONN
 *---------------------------------------------------------------------------*
 * Description: Restore the network context.
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_RESTORENWCONN(void)
{
	return AtCmd_SendCommand( (char *)"AT+RESTORENWCONN\r\n");
}



/*--------------------------------  Command - Response  -----------------------------------------*/

/*---------------------------------------------------------------------------*
 * AtCmd_SendCommand
 *---------------------------------------------------------------------------*
 * Description: Sends the AT command to GS2200, waits for a response.
 *              Response is pushed into RxBuffer.
 * Inputs: char *aString -- AT command to send
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_SendCommand(char *command)
{
	SPI_RESP_STATUS_E s;

#ifdef ATCMD_DEBUG_ENABLE
	ConsolePrintf( ">%s\n", command );
#endif

	/* Send the command to GS2200 */
	s = WiFi_Write((char *)command, strlen(command));

	if( s == SPI_RESP_STATUS_OK )
		return AtCmd_RecvResponse();
	else
		return ATCMD_RESP_SPI_ERROR;
}

/*---------------------------------------------------------------------------*
 * AtCmd_checkResponse
 *---------------------------------------------------------------------------*
 * Description: Check the completion of Response
 * Inputs: const char *pBuffer -- Line of data to check
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_checkResponse(const char *pBuffer)
{
	const char *p;
	uint8_t numSpaces;

	if( (strstr( pBuffer, "OK") != NULL)) {
		return ATCMD_RESP_OK;
	}
	else if (strstr((const char *)pBuffer, "ERROR: SOCKET FAILURE") != NULL) {
		return ATCMD_RESP_ERROR_SOCKET_FAILURE;
	}
	else if ((strstr((const char *)pBuffer, "ERROR: IP CONFIG FAIL") != NULL)) {
		return ATCMD_RESP_ERROR_IP_CONFIG_FAIL;
	}
	else if ((strstr((const char *)pBuffer, "ERROR") != NULL)) {
		return ATCMD_RESP_ERROR;
	}
	else if ((strstr((const char *)pBuffer, "INVALID INPUT") != NULL)) {
		return ATCMD_RESP_INVALID_INPUT;
	}
	else if ((strstr((const char *)pBuffer, "INVALID CID") != NULL)) {
		return ATCMD_RESP_INVALID_CID;
	}
	else if ((strstr((const char *)pBuffer, "DISASSOCIATED") != NULL)) {
		return ATCMD_RESP_DISASSOCIATION_EVENT;
	}
	else if ((strstr((const char *)pBuffer, "APP Reset-APP SW Reset")) != NULL) {
		return ATCMD_RESP_RESET_APP_SW;
	}
	else if ((strstr((const char *)pBuffer, "DISCONNECT")) != NULL) {
		return ATCMD_RESP_DISCONNECT;
	}
	else if ((strstr((const char *)pBuffer, "Disassociation Event")) != NULL) {
		return ATCMD_RESP_DISASSOCIATION_EVENT;
	}
	else if ((strstr((const char *)pBuffer, "Out of StandBy-Alarm")) != NULL) {
		return ATCMD_RESP_OUT_OF_STBY_ALARM;
	}
	else if ((strstr((const char *)pBuffer, "Out of StandBy-Timer")) != NULL) {
		return ATCMD_RESP_OUT_OF_STBY_TIMER;
	}
	else if ((strstr((const char *)pBuffer, "External Reset")) != NULL) {
		return ATCMD_RESP_EXTERNAL_RESET;
	}
	else if ((strstr((const char *)pBuffer, "Out of Deep Sleep")) != NULL) {
		return ATCMD_RESP_OUT_OF_DEEP_SLEEP;
	}
	else if ((strstr((const char *)pBuffer, "Serial2WiFi APP")) != NULL) {
		return ATCMD_RESP_NORMAL_BOOT_MSG;
	}
	else if ((pBuffer[0] == 'A') && (pBuffer[1] == 'T') && (pBuffer[2] == '+')) {
		/* Echoed back AT Command, if Echo is enabled.  "AT+" . */
		return ATCMD_RESP_UNMATCH;
	}
	else if (strstr((const char *)pBuffer, "CONNECT ") != NULL) {
		/* Determine if this CONNECT line is of the type CONNECT <server CID> <new CID> <ip> <port> */
		/* by counting spaces in between the words. Used for TCP Server. */
		p = pBuffer;
		numSpaces = 0;
		while ((*p) && (*p != '\n')) {
			if (*p == ' ')
				numSpaces++;
			if (numSpaces >= 4)
				return ATCMD_RESP_TCP_SERVER_CONNECT;
			p++;
		}
	}
	
	return ATCMD_RESP_UNMATCH;
}

/*---------------------------------------------------------------------------*
 * AtCmd_ParseRcvData
 *---------------------------------------------------------------------------*
 * Description: Parse the received charcter one by one
 * Inputs: uint8_t rxData -- Character to process
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_ParseRcvData(uint8_t *ptr)
{
	/* receive data handling state */
	static ATCMD_FSM_E rcv_state = ATCMD_FSM_START;
	static uint8_t  *head;
	int msgSize;
	int i;
	
	static uint8_t  getCid;
	static bool  spcFlag, htabFlag;
	static uint16_t dataLen = 0;
	static uint8_t dataLenCount = 0;
	
	ATCMD_RESP_E resp = ATCMD_RESP_UNMATCH;
	
#ifdef ATCMD_DEBUG_ENABLE
	if ((isprint(*ptr)) || (isspace(*ptr))){
		ConsoleByteSend( *ptr );
	}
#endif    
	/* Parse the received data */
	switch (rcv_state) {
	case ATCMD_FSM_START:
		switch (*ptr) {
		case ATCMD_CR:
		case ATCMD_LF:
			/* CR and LF at the begining, just ignore it */
			break;
			
		case ATCMD_ESC:
			/* ESCAPE sequence detected */
			rcv_state = ATCMD_FSM_ESC_START;
			getCid = 0;
			dataLenCount=0;
			dataLen = 0;
			break;

		default:
			/* Probably, start of the response string */
			head = ptr;
			/* Release memory */
			for( i=0; i<RespBuffer_Index; i++ )
				if( RespBuffer[i] )
					free( RespBuffer[i] );
			
			RespBuffer_Index = 0;
			rcv_state = ATCMD_FSM_RESPONSE;
			break;
		}
		break;

	case ATCMD_FSM_RESPONSE:
		if (ATCMD_LF == *ptr) {
			/* LF detected - Messages from GS2200 are terminated with LF character */
			/* Copy the string to the RespBuffer */
			if( RespBuffer_Index < NUM_OF_RESPBUFFER ){
				if( ptr <= head ){
					ConsoleLog( "Something wrong in buffer handling!\r\n" );
					ConsoleLog( "ptr should be larger than head!\r\n" );
					resp = ATCMD_RESP_ERROR_BUFFER_HANDLING;
					rcv_state = ATCMD_FSM_START;					
					return resp;
				}
				msgSize = ptr - head;
				RespBuffer[RespBuffer_Index] = (uint8_t *)malloc(msgSize+1);
				if( RespBuffer[RespBuffer_Index] == NULL ){
					ConsoleLog( "Out of memory!\r\n" );
					rcv_state = ATCMD_FSM_START;
					resp = ATCMD_RESP_NO_MORE_MEMORY;
					rcv_state = ATCMD_FSM_START;					
					return resp;
				}
				memcpy( RespBuffer[RespBuffer_Index], head, msgSize );
				/* terminate string with NULL */
				*( RespBuffer[RespBuffer_Index]+msgSize) = 0;
				resp = AtCmd_checkResponse( (const char *)RespBuffer[RespBuffer_Index] );
				head = ptr+1;
				RespBuffer_Index++;

				if (ATCMD_RESP_UNMATCH != resp) {
					/* command echo or end of response detected */
					/* Now reset the  state machine */
					rcv_state = ATCMD_FSM_START;
				}
			}
			else{ // No more RespBuffer!! but need to parse the rest of string
				resp = AtCmd_checkResponse( (const char *)ptr );
				if (ATCMD_RESP_UNMATCH != resp) {
					rcv_state = ATCMD_FSM_START;
				}
			}
		}
		break;
		
	case ATCMD_FSM_ESC_START:
		if ( 'Z' == *ptr) {
			/* Bulk data handling start */
			/* <Esc>Z<Cid><Data Length xxxx 4 ascii char><data>   */
			rcv_state = ATCMD_FSM_BULK_DATA;
		}
		else if ( 'H' == *ptr) {
			/* HTTP data handling start */
			/* <Esc>H<Cid><Data Length xxxx 4 ascii char><data>   */
			rcv_state = ATCMD_FSM_BULK_DATA;
		}
		else if ('O' == *ptr) {
			/* ESC command response OK */
			/* Note: No action is needed. */
			rcv_state = ATCMD_FSM_START;
			resp = ATCMD_RESP_ESC_OK;
		}
		else if ( 'F' == *ptr) {
			/* ESC command response FAILED */
			/* Wrong CID, you need to check CID */
			rcv_state = ATCMD_FSM_START;
			resp = ATCMD_RESP_ESC_FAIL;
		}
		else if ( 'y' == *ptr) {
			/* Start of UDP data */
			/* ESC y  cid IP_addr <SPC> Port <HT> <Length 4digits> data */
			spcFlag = false;
			htabFlag = false;
			rcv_state = ATCMD_FSM_UDP_BULK_DATA;
		}
		else {
			/* ESC sequence parse error !  */
			/* Reset the receive buffer */
			rcv_state = ATCMD_FSM_START;
		}
		break;
		
	case ATCMD_FSM_BULK_DATA:
		if( !getCid ){
			/* Store the CID */
			WiFi_StoreESCBuffer( *ptr );
			getCid = 1;
		}
		else if( dataLenCount < 4 ){
			/* Calculate Data Length */
			dataLen = (dataLen * 10) + *ptr - '0';
			dataLenCount++;
		}
		else{
			/* Now read actual data */
			WiFi_StoreESCBuffer( *ptr );
			resp = ATCMD_RESP_BULK_DATA_RX;
			dataLen--;
			if( !dataLen ){
				rcv_state = ATCMD_FSM_START;
				resp = ATCMD_RESP_BULK_DATA_RX;
			}
		}
		break;

	case ATCMD_FSM_UDP_BULK_DATA:
		/* ESC y <CID><IP_addr>SPC<Port>HT<Length 4digits> data */
		if( !getCid ){
			/* Store the CID */
			WiFi_StoreESCBuffer( *ptr );
			getCid = 1;
		}
		else if( !spcFlag || !htabFlag ){
			/* Find <SPC>, then <HT> */
			if( *ptr == 0x20 ){
				spcFlag = true;
			}
			else if( *ptr == 0x09 && spcFlag ){
				htabFlag = true;
			}
			WiFi_StoreESCBuffer( *ptr );
		}
		else if( dataLenCount < 4 ){
			/* Calculate Data Length */
			dataLen = (dataLen * 10) + *ptr - '0';
			dataLenCount++;
		}
		else{
			/* Now read actual data */
			WiFi_StoreESCBuffer( *ptr );
			dataLen--;
			if( !dataLen ){
				rcv_state = ATCMD_FSM_START;
				resp = ATCMD_RESP_UDP_BULK_DATA_RX;
			}
		}
		break;
		

	default:
		/* This case will not be executed */
		rcv_state = ATCMD_FSM_START;
		break;
	}

	return resp;
}

/*---------------------------------------------------------------------------*
 * AtCmd_RecvResponse
 *---------------------------------------------------------------------------*
 * Description: Wait for a response after sending a command. Keep parsing the
 *		data until a response is found.
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_RecvResponse(void)
{
	SPI_RESP_STATUS_E s;
	ATCMD_RESP_E resp;
	uint16_t rxDataLen;
	uint8_t *p;

	
	/* Reset the message ID */
	resp = ATCMD_RESP_UNMATCH;
	
	s = WiFi_Read( RxBuffer, &rxDataLen );

	if( s == SPI_RESP_STATUS_TIMEOUT ){
#ifdef ATCMD_DEBUG_ENABLE
		ConsoleLog("SPI: Timeout");
#endif
		return ATCMD_RESP_TIMEOUT;
	}
	
	if( s == SPI_RESP_STATUS_ERROR ){
#ifdef ATCMD_DEBUG_ENABLE
		/* Zero data length, this should not happen */
		ConsoleLog("GS2200 responds Zero data length");
#endif
		return ATCMD_RESP_ERROR;
	}
	
	p = RxBuffer;
	while( rxDataLen-- ){
		/* Parse the received data */
		resp = AtCmd_ParseRcvData( p++ );
	}             

#ifdef ATCMD_DEBUG_ENABLE
	ConsolePrintf( "GS Response: %d\r\n", resp );
#endif    
	return resp;

}	



/*--------------------------------  Layer 4 Communication  -----------------------------------------*/

/*---------------------------------------------------------------------------*
 * AtCmd_SendBulkData
 *---------------------------------------------------------------------------*
 * Description: Send bulk data (TCP Server/Client, UDP Client)
 *              <ESC><'Z'><cid><Data length (4 byte ASCII)><Data>
 * Inputs:
 *      uint8_t cid -- Connection ID
 *      const char *pTxData -- Data to send to the TCP connection
 *      uint16_t dataLen -- Length of data to send
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_SendBulkData(uint8_t cid, const void *txBuf, uint16_t dataLen)
{
	#define HEADERSIZE 7
	SPI_RESP_STATUS_E s;
	char digits[5]; 
	char cmd[10];

	memset( cmd, 0, sizeof(cmd) );

	/* Convert the data length to 4 digit bytes */
	ConvertNumberTo4DigitASCII(dataLen, digits);
	/* Construct header part of the bulk data string */
	/*<Esc><'Z'><Cid><Data Length (4 byte ASCII)> <Data> */
	sprintf( cmd, "%cZ%c%s", ATCMD_ESC, cid, digits );

	memcpy( TxBuffer, cmd, HEADERSIZE );
	memcpy( TxBuffer+HEADERSIZE, txBuf, dataLen );
	
	/* Send the bulk data to GS2200 */
	s = WiFi_Write( (char *)TxBuffer, dataLen+HEADERSIZE );

	if( s == SPI_RESP_STATUS_OK )
		return ATCMD_RESP_OK;
	else
		return ATCMD_RESP_SPI_ERROR;

}


/*---------------------------------------------------------------------------*
 * AtCmd_UDP_SendBulkData
 *---------------------------------------------------------------------------*
 * Description: Send bulk data in UDP server
 *              <ESC><'Y'><cid><ip>:<port>:<Data Length><Data>
 * Inputs: uint8_t cid -- Connection ID
 *         const void *txBuf -- Data to send to the UDP connection
 *         uint16_t dataLen -- Data Length
 *         char *pUdpClientIP -- Client IP address
 *         uint16_t udpClientPort -- Port of UDP client
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_UDP_SendBulkData(
        uint8_t cid,
        const void *txBuf,
        uint16_t dataLen,
        const char *pUdpClientIP,
        uint16_t udpClientPort)
{
	ATCMD_RESP_E resp=ATCMD_RESP_UNMATCH;
	SPI_RESP_STATUS_E s;
	char digits[5];
	char cmd[30];

	if (ATCMD_INVALID_CID != cid) {

		memset( cmd, 0, sizeof(cmd) );

		ConvertNumberTo4DigitASCII(dataLen, digits);
		/* Construct header part of the bulk data string */
		/*<Esc><'Y'><cid><ip>:<port>:<Data Length><Data> */
		sprintf( cmd, "%cY%c%s:%d:%s", ATCMD_ESC, cid, pUdpClientIP, udpClientPort, digits );
		
		memcpy( TxBuffer, cmd, strlen(cmd) );
		memcpy( TxBuffer+strlen(cmd), txBuf, dataLen );
		
		/* Send the bulk data to GS2200 */
		s = WiFi_Write( (char *)TxBuffer, dataLen+HEADERSIZE );
		
		if( s == SPI_RESP_STATUS_OK )
			resp = ATCMD_RESP_OK;
		else
			resp = ATCMD_RESP_SPI_ERROR;
	}

	return resp;
}


/*---------------------------------------------------------------------------*
 * WaitForTCPConnection
 *---------------------------------------------------------------------------*
 * Description: Waits for and parses a TCP message.
 * Inputs:
 *      ATLIBGS_TCPConnection *connection -- Connection structure to fill
 *      uint32_t timeout -- Timeout in milliseconds, 0 for no timeout
 * Outputs:
 *      ATLIBGS_MSG_ID_E -- returns ATLIBGS_MSG_ID_RESPONSE_TIMEOUT if timeout,
 *          or ATLIBGS_MSG_ID_DATA_RX if TCP message found.
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E WaitForTCPConnection( char *cid, uint32_t timeout )
{
	ATCMD_RESP_E resp;
	uint32_t start = millis();
	char *p;
	char *tokens[6];
	uint16_t numTokens;

	/* wait until message received or timeout*/
	while (1) {
		if( (msDelta(start) >= timeout) && (timeout) )
			return ATCMD_RESP_TIMEOUT;

		if( Get_GPIO37Status() ){
			resp = AtCmd_RecvResponse();
               
			if( ATCMD_RESP_TCP_SERVER_CONNECT == resp ){
				p = strstr( (const char*)RespBuffer[0], "CONNECT");
				if( p ){
					numTokens = ParseIntoTokens(p, ' ', tokens, 6);
					if (numTokens >= 5) {
						*cid = *tokens[2];
						return resp;
					}
				}
				else{
					return ATCMD_RESP_ERROR;
				}
			}
		}
	}
	
}


/*---------------------------------  MQTT Communication  --------------------------------------*/

/*---------------------------------------------------------------------------*
 * AtCmd_MQTTCONNECT
 *---------------------------------------------------------------------------*
 * Description: Create a MQTT netowork connection, send MQTT CONNECT packet
 * Inputs: char *cid -- Connection ID is stored if connection established
 *         char *host -- MQTT broker name or IP address
 *         char *port -- MQTT port number
 *         char *clientID -- ID name, this can be any string
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_MQTTCONNECT( char *cid, char *host, char *port, char *clientID, char *UserName, char *Password )
{
	ATCMD_RESP_E resp=ATCMD_RESP_UNMATCH;
	char cmd[180];
	char *result;

	if( UserName==NULL )
		sprintf( cmd, "AT+MQTTCONNECT=%s,%s,%s\r\n", host, port, clientID );
	else
		sprintf( cmd, "AT+MQTTCONNECT=%s,%s,%s,%s,%s,0\r\n", host, port, clientID, UserName, Password );
		
	resp = AtCmd_SendCommand( cmd );

	if( resp == ATCMD_RESP_OK && RespBuffer_Index ) {
		if( (result = strstr( (const char*)RespBuffer[0], "IP")) != NULL) {
			/* CID must be in the second line of the response */
			*cid = Search_CID( RespBuffer[1] );
#ifdef ATCMD_DEBUG_ENABLE
			ConsolePrintf( "MQTT Server CID: %c\r\n", *cid );
			ConsolePrintf( "MQTT Server IP Address: %s\r\n", result+3 );
#endif    
		}
		else{
			/* IP address is provided */
			/* CID must be in the first line of the response */
			*cid = Search_CID( RespBuffer[0] );
		}
	}

	return resp;
}

/*---------------------------------------------------------------------------*
 * AtCmd_MQTTPUBLISH
 *---------------------------------------------------------------------------*
 * Description: Send MQTT application message
 * Inputs: char cid -- Connection ID of MQTT
 *         char *topic -- TOPIC on MQTT broker
 *         ATCMD_MQTTparams mqtt -- structure of {length, QoS, retain, message}
 * Note: Only ASCII message is supported
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_MQTTPUBLISH( char cid, ATCMD_MQTTparams mqtt )
{
        #define BUFLEN 180
	char cmd[BUFLEN];
	ATCMD_RESP_E resp=ATCMD_RESP_UNMATCH;
	SPI_RESP_STATUS_E s;


	if( strlen(mqtt.topic) < BUFLEN - 28 ){
		sprintf( cmd, "AT+MQTTPUBLISH=%c,%s,%d,%d,%d\r\n", cid, mqtt.topic, mqtt.len, mqtt.QoS, mqtt.retain );
		if( ATCMD_RESP_OK == AtCmd_SendCommand( cmd ) ){
			/* MQTT Publish */
			/*<Esc><'N'><cid><Data> */
			sprintf( (char *)TxBuffer, "%cN%c%s", ATCMD_ESC, cid, mqtt.message );
			/* Send the bulk data to GS2200 */
			s = WiFi_Write( (char *)TxBuffer, strlen((char *)TxBuffer) );
			
			if( s == SPI_RESP_STATUS_OK )
				resp = ATCMD_RESP_OK;
			else
				resp = ATCMD_RESP_SPI_ERROR;
			
			return resp;
		}
	}
	else
		return ATCMD_RESP_INPUT_TOO_LONG;

	return resp;
}

/*---------------------------------------------------------------------------*
 * AtCmd_MQTTSUBSCRIBE
 *---------------------------------------------------------------------------*
 * Description: Send MQTT application message
 * Inputs: char cid -- Connection ID of MQTT
 *         char *topic -- TOPIC on MQTT broker
 *         ATCMD_MQTTparams mqtt -- structure of {length, QoS, retain, message}
 * Note: Only ASCII message is supported
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_MQTTSUBSCRIBE( char cid, ATCMD_MQTTparams mqtt )
{
	#define BUFLEN 180

	char cmd[BUFLEN];
	ATCMD_RESP_E resp=ATCMD_RESP_UNMATCH;
	SPI_RESP_STATUS_E s;


	if( strlen(mqtt.topic) < BUFLEN - 28 ){
		sprintf( cmd, "AT+MQTTSUBSCRIBE=%c,%s,%d\r\n", cid, mqtt.topic, mqtt.QoS );
		if( ATCMD_RESP_OK == AtCmd_SendCommand( cmd ) ){
			if( s == SPI_RESP_STATUS_OK )
				resp = ATCMD_RESP_OK;
			else
				resp = ATCMD_RESP_SPI_ERROR;

			return resp;
		}
	}
	else
		return ATCMD_RESP_INPUT_TOO_LONG;

	return resp;
}

/*---------------------------------------------------------------------------*
 * AtCmd_RecieveMQTTData
 *---------------------------------------------------------------------------*
 * Description: Recieve MQTT Subscribed data
 * Inputs: String& data -- MQTT Subscribed data
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E  AtCmd_RecieveMQTTData( String& data )
{
	SPI_RESP_STATUS_E s;
	ATCMD_RESP_E resp;
	uint16_t rxDataLen;

	/* Reset the message ID */
	resp = ATCMD_RESP_UNMATCH;

	s = WiFi_Read( RxBuffer, &rxDataLen );

	if( s == SPI_RESP_STATUS_TIMEOUT ){
#ifdef ATCMD_DEBUG_ENABLE
		ConsoleLog("SPI: Timeout");
#endif
		return ATCMD_RESP_TIMEOUT;
	}

	if( s == SPI_RESP_STATUS_ERROR ){
#ifdef ATCMD_DEBUG_ENABLE
		/* Zero data length, this should not happen */
		ConsoleLog("GS2200 responds Zero data length");
#endif
		return ATCMD_RESP_ERROR;
	}

	char* p = RxBuffer;
	while( rxDataLen-- ){
		/* Parse the received data */
		resp = AtCmd_ParseRcvData( p++ );
	}


	if(resp == ATCMD_RESP_DISCONNECT ){
		return resp;
	}

	String rxdata = RxBuffer;
	String size = rxdata.substring( (1+1+1+1+4), (1+1+1+1+4+4) );
	String topic = rxdata.substring( (1+1+1+1+4+4), rxdata.indexOf(' ') );
	data = rxdata.substring( rxdata.indexOf(' ')+1, rxdata.indexOf(' ')+1+size.toInt() );

#ifdef ATCMD_DEBUG_ENABLE

	Serial.println();
	Serial.print("Recieve topic: ");
 	Serial.println(topic);
	Serial.print("Recieve size: ");
	Serial.println(size);
	Serial.print("Recieve data: ");
	Serial.println(data);

#endif

	return ATCMD_RESP_OK;

}

/*---------------------------------  HTTP Communication  --------------------------------------*/

/*---------------------------------------------------------------------------*
 * AtCmd_HTTPOPEN
 *---------------------------------------------------------------------------*
 * Description: Open an HTTP client connection
 * Inputs: char *cid -- Connection ID is stored, if connection established
 *         char *host -- HTTP server name or IP address
 *         char *port -- HTTP port number
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_HTTPOPEN( char *cid, const char *host, const char *port )
{
	ATCMD_RESP_E resp=ATCMD_RESP_UNMATCH;
	char cmd[80];
	char *result;
	
	sprintf( cmd, "AT+HTTPOPEN=%s,%s\r\n", host, port );
	resp = AtCmd_SendCommand( cmd );

	if( resp == ATCMD_RESP_OK && RespBuffer_Index ) {
		if( (result = strstr( (const char*)RespBuffer[0], "IP")) != NULL) {
			/* CID must be in the second line of the response */
			*cid = Search_CID( RespBuffer[1] );
#ifdef ATCMD_DEBUG_ENABLE
			ConsolePrintf( "HTTP Server CID: %c\r\n", *cid );
			ConsolePrintf( "HTTP Server IP Address: %s\r\n", result+3 );
#endif    
		}
		else{
			/* IP address is provided */
			/* CID must be in the first line of the response */
			*cid = Search_CID( RespBuffer[0] );
		}
	}

	if( *cid == ATCMD_INVALID_CID )
		resp = ATCMD_RESP_INVALID_CID;

	return resp;
}

/*---------------------------------------------------------------------------*
 * AtCmd_HTTPSOPEN
 *---------------------------------------------------------------------------*
 * Description: Open an HTTPS client connection
 * Inputs: char *cid -- Connection ID is stored, if connection established
 *         char *host -- HTTP server name or IP address
 *         char *port -- HTTP port number
 *         const char *port -- CA Name
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_HTTPSOPEN( char *cid, const char *host, const char *port, const char *ca_name)
{
	ATCMD_RESP_E resp=ATCMD_RESP_UNMATCH;
	char cmd[80];
	char *result;
	
	sprintf( cmd, "AT+HTTPOPEN=%s,%s,1,%s\r\n", host, port,ca_name );
	resp = AtCmd_SendCommand( cmd );

	if( resp == ATCMD_RESP_OK && RespBuffer_Index ) {
		if( (result = strstr( (const char*)RespBuffer[0], "IP")) != NULL) {
			/* CID must be in the second line of the response */
			*cid = Search_CID( RespBuffer[1] );
#ifdef ATCMD_DEBUG_ENABLE
			ConsolePrintf( "HTTP Server CID: %c\r\n", *cid );
			ConsolePrintf( "HTTP Server IP Address: %s\r\n", result+3 );
#endif    
		}
		else{
			/* IP address is provided */
			/* CID must be in the first line of the response */
			*cid = Search_CID( RespBuffer[0] );
		}
	}

	if( *cid == ATCMD_INVALID_CID )
		resp = ATCMD_RESP_INVALID_CID;

	return resp;
}


/*---------------------------------------------------------------------------*
 * AtCmd_HTTPCONF
 *---------------------------------------------------------------------------*
 * Description: Configure the HTTP patameters
 * Inputs: ATCMD_HTTP_HEADER param -- HTTP header parameter
 *         char *val -- value of header
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_HTTPCONF( ATCMD_HTTP_HEADER_E param, const char *val )
{
	ATCMD_RESP_E resp=ATCMD_RESP_UNMATCH;
	char cmd[120];
	
	sprintf( cmd, "AT+HTTPCONF=%d,%s\r\n", param, val );
	resp = AtCmd_SendCommand( cmd );

	return resp;
}


/*---------------------------------------------------------------------------*
 * AtCmd_HTTPSEND
 *---------------------------------------------------------------------------*
 * Description: GET/POST HTTP data
 * Inputs: char cid -- Connection ID of MQTT
 *         ATCMD_HTTP_METHOD type -- type of method
 *         uint8_t timeout -- timeout of RESPONSE
 *         char *page -- the page being accessed
 *         uint16_t size -- actual content size
 * Note: Support GET, POST method
 * Note: Support only ASCII message
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_HTTPSEND( char cid, ATCMD_HTTP_METHOD_E type, uint8_t timeout, const char *page, const char *msg, uint32_t size )
{
	char cmd[120];
	ATCMD_RESP_E resp=ATCMD_RESP_UNMATCH;
	SPI_RESP_STATUS_E s;

	if( HTTP_METHOD_GET==type ){
		sprintf( cmd, "AT+HTTPSEND=%c,%d,%d,%s\r\n", cid, type, timeout, page );
		return AtCmd_SendCommand( cmd );
	}
	else if( HTTP_METHOD_POST==type ){
		sprintf( cmd, "AT+HTTPSEND=%c,%d,%d,%s,%ld\r\n", cid, type, timeout, page, size );
		if( ATCMD_RESP_OK == AtCmd_SendCommand( cmd ) ){
			/* HTTP POST : <Esc><'H'><cid><Data> */
			
			/* Send <Esc><'H'><cid> at first */
			sprintf( (char *)TxBuffer, "%cH%c", ATCMD_ESC, cid );
			/* Send the bulk data to GS2200 */
			s = WiFi_Write( (char *)TxBuffer, 3 );
			
			if( s != SPI_RESP_STATUS_OK ){
				resp = ATCMD_RESP_SPI_ERROR;
				return resp;
			}
			
			ConsoleLog( "Start to send data body");
			/* Send Data */
			if( size <= SPI_MAX_SIZE ){
				do{
					s = WiFi_Write( msg, size );
					if( s != SPI_RESP_STATUS_OK )
						delay(100);
				}while( s != SPI_RESP_STATUS_OK );
				
			}
			else{
				while( size>SPI_MAX_SIZE ){
					memcpy( TxBuffer, msg, SPI_MAX_SIZE );
					s = WiFi_Write( (char *)TxBuffer, SPI_MAX_SIZE );
			
					if( s != SPI_RESP_STATUS_OK ){
						delay(100);
						continue;
					}
					msg += SPI_MAX_SIZE;
					size -= SPI_MAX_SIZE;
					ConsolePrintf( "%d remains\r\n", size);
				}
				
				if( size ){
					do{
						s = WiFi_Write( msg, size );
						if( s != SPI_RESP_STATUS_OK )
							delay(100);
					}while( s != SPI_RESP_STATUS_OK );
				}
				
			}
			
			ConsoleLog( "Send data body DONE");
			return ATCMD_RESP_OK;
		}
	}
	else{
		ConsolePrintf( "Not support HTTP method : %d\r\n", type );
		return ATCMD_RESP_ERROR;
	}
	
	return resp;

}


/*---------------------------------------------------------------------------*
 * AtCmd_HTTPCLOSE
 *---------------------------------------------------------------------------*
 * Description: Clost HTTP connection
 * Inputs: char cid -- Connection ID to be closed
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_HTTPCLOSE( char cid )
{
	ATCMD_RESP_E resp=ATCMD_RESP_UNMATCH;
	char cmd[20];
	
	sprintf( cmd, "AT+HTTPCLOSE=%c\r\n", cid );
	resp = AtCmd_SendCommand( cmd );

	return resp;
}


/*---------------------------------  Advanced Services  --------------------------------------*/

/*---------------------------------------------------------------------------*
 * AtCmd_DNSLOOKUP
 *---------------------------------------------------------------------------*
 * Description: Retrieve an IP address from a host name
 * Inputs: char *cid -- Connection ID is stored, if connection established
 *         char *host -- HTTP server name or IP address
 *         char *port -- HTTP port number
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_DNSLOOKUP( char *host, char *ip )
{
	ATCMD_RESP_E resp=ATCMD_RESP_UNMATCH;
	char cmd[80];
	char *result, *last;
	int i;
	
	sprintf( cmd, "AT+DNSLOOKUP=%s\r\n", host );
	if( ATCMD_RESP_OK == (resp=AtCmd_SendCommand( cmd )) ){
		if( (result = strstr( (const char*)RespBuffer[0], "IP:")) != NULL) {
			/* store IP address */
			result += 3; // this location must be the start of IP address
			for( i=0, last=result; i<16; i++, last++ )
				if( !( (*last >= '0' && *last <= '9') || *last=='.' ) )
					break;

			memcpy( ip, result, (last-result) );
		}
		else{
			/* IP address is not found... */
			resp = ATCMD_RESP_ERROR;
		}
		
	}

	return resp;
}


/*---------------------------------  Advanced Commands  --------------------------------------*/

/*---------------------------------------------------------------------------*
 * AtCmd_APCLIENTINFO
 *---------------------------------------------------------------------------*
 * Description: Get the information about the clients associated to GS2200
 *              in Limited AP mode.
 *     example output
 *          No.Of Stations Connected=1
 *          No     MacAddr                   IP
 *          1      2c:44:01:c5:e7:df         192.168.1.2
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_APCLIENTINFO(void)
{
	ATCMD_RESP_E resp;
	int i;

	resp = AtCmd_SendCommand( (char *)"AT+APCLIENTINFO=?\r\n");

	if( resp == ATCMD_RESP_OK ){
		for( i=0; i<RespBuffer_Index-1; i++) {
			ConsolePrintf( "%s", RespBuffer[i]);
		}
	}

	return resp;
}

#ifndef SUBCORE
/*---------------------------------------------------------------------------*
 * AtCmd_TCERTADD
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_TCERTADD( char* name, int format, int location, File fp )
{
	ATCMD_RESP_E resp=ATCMD_RESP_UNMATCH;
	SPI_RESP_STATUS_E s;
	char cmd[80];
	char *result;
	
	if (!fp.available())
		return ATCMD_RESP_INVALID_INPUT;

	
	if( fp.size() < TXBUFFER_SIZE - 2 ){
		sprintf( cmd, "AT+TCERTADD=%s,%d,%d,%d\r\n", name, format, fp.size(), location );
		if( ATCMD_RESP_OK == AtCmd_SendCommand( cmd ) ){
			TxBuffer[0] = ATCMD_ESC;
			TxBuffer[1] = 'W';
			fp.read(&TxBuffer[2], fp.size());
			s = WiFi_Write( (char *)TxBuffer, fp.size()+2 );

			if( s == SPI_RESP_STATUS_OK ){
		    	puts("ATCMD_RESP_OK");
				resp = ATCMD_RESP_OK;
			}else{
		    	puts("ATCMD_RESP_SPI_ERROR");
				resp = ATCMD_RESP_SPI_ERROR;
			}
			return resp;
		}
	} else {
		puts("ATCMD_RESP_INPUT_TOO_LONG");
		return ATCMD_RESP_INPUT_TOO_LONG;
	}

	return AtCmd_RecvResponse();

}
#endif 

/*---------------------------------------------------------------------------*
 * AtCmd_SETTIME
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_SETTIME(char* time)
{
	ATCMD_RESP_E resp=ATCMD_RESP_UNMATCH;
	char cmd[80];
    sprintf( cmd, "AT+SETTIME=%s\r\n", time );
	resp = AtCmd_SendCommand( cmd );
	return resp;
}

/*---------------------------------------------------------------------------*
 * AtCmd_SSLCONF
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_SSLCONF(int size)
{
	ATCMD_RESP_E resp=ATCMD_RESP_UNMATCH;
	char cmd[80];
    sprintf( cmd, "AT+SSLCONF=1,%d\r\n", size );
	resp = AtCmd_SendCommand( cmd );
	return resp;
}

/*---------------------------------------------------------------------------*
 * AtCmd_LOGLVL
 *---------------------------------------------------------------------------*/
ATCMD_RESP_E AtCmd_LOGLVL(int level)
{
	ATCMD_RESP_E resp=ATCMD_RESP_UNMATCH;
	char cmd[80];
    sprintf( cmd, "AT+LOGLVL=%d\r\n", level );
	resp = AtCmd_SendCommand( cmd );
	return resp;
}


/*-------------------------------------------------------------------------*
 * End of File:  AtCmd.cpp
 *-------------------------------------------------------------------------*/
