/*
 *  TelitWiFi.cpp - Telit GS2000 Series WiFi Module Operations Include File
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

#include <Arduino.h>
#include <GS2200AtCmd.h>

typedef struct {
	ATCMD_MODE_E  mode;
	ATCMD_PSAVE_E psave;
} TWIFI_Params;


#define OK    0
#define FAIL  1


/**
 * @class TelitWiFi
 * @brief Telit GS2000 Series Module Operations
 *
 * @details GS2000 Series WiFi module Initialization & Association in Station/Limited-AP mode
 */
class TelitWiFi
{
public:
	TelitWiFi();

	~TelitWiFi();

	/**
	 *  GS2000 Initialization 
	 */
	int begin(TWIFI_Params params);
	
	/**
	 *  AP association (station mode), AP creation (Limited-AP mode)
	 */
	int connect(const char *ssid, const char *passphrase);
	int connect(const char *ssid, const char *passphrase, uint8_t channel);

};
