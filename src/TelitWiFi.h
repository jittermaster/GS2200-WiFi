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

#ifndef _TELITWIFI_H_
#define _TELITWIFI_H_

#include <Arduino.h>
#include <GS2200AtCmd.h>
#include <GS2200Hal.h>

typedef struct {
	ATCMD_MODE_E  mode;
	ATCMD_PSAVE_E psave;
} TWIFI_Params;


/**
 * @class TelitWiFi
 * @brief Telit GS2000 Series Module Operations
 *
 * @details GS2000 Series WiFi module Initialization & Association in Station/Limited-AP mode
 */
class TelitWiFi
{
public:

	typedef enum
	{
		OK = 0,
		FAIL = 1,
	} TelitResult;

	TelitWiFi();

	~TelitWiFi();

	/**
	 *  GS2000 Initialization 
	 */
	int begin(TWIFI_Params params);

	/**
	 *  AP association (station mode), AP creation (Limited-AP mode)
	 */
	int activate_station(const String& ssid, const String& passphrase);
	int activate_ap(const String& ssid, const String& passphrase, uint8_t channel);

	/**
	 * Connect TCP server
	 */
	char connect(const String& ip, const String& port);

	/**
	 * Connect TCP server
	 */
	bool connected(char cid);

	/**
	 * Disonnect to TCP server
	 */
	void stop(char cid);

	/**
	 * Send data to TCP server
	 */
	bool write(char cid, const uint8_t* data, int length);

	/**
	 *  Available TCP read
	 */
	bool available();

	/**
	 * Send data to TCP server
	 */
	int read(char cid, uint8_t* data, int length);

};

#endif /*_TELITWIFI_H_*/
