/*
 *  HTTP Library for GS2200
 *  Copyright 2023 Spresense Users
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "HttpGs2200.h"

// #define HTTP_DEBUG

#ifdef HTTP_DEBUG
#define DBG(...) { Serial.print(__VA_ARGS__); }
#define ERR(...) { Serial.print(__VA_ARGS__); }
#else
#define DBG(...)
#define ERR(...)
#endif /* HTTP_DEBUG */


bool HttpGs2200::begin(TelitWiFi* wifi, HTTPGS2200_HostParams* params)
{
#ifdef HTTP_DEBUG
    Serial.println("Initialize HTTP");
#endif

  mWifi = wifi;
  mCid = ATCMD_INVALID_CID;

  mData.host = params->host;
  mData.port = params->port;
  
#ifdef HTTP_DEBUG
  Serial.println("Server: " + (String)mData.host + ", Port: " + mData.port);
#endif

  return true;
}

bool HttpGs2200::config(ATCMD_HTTP_HEADER_E param, const char *val)
{
	ATCMD_RESP_E resp = ATCMD_RESP_UNMATCH;
	bool result = false;

	resp = AtCmd_HTTPCONF(param, val);
	if (ATCMD_RESP_OK == resp) {
		result = true;
	} else {
		printf("config error! err resp = %d\n", resp);
		result = false;
	}

  return result;
}

bool HttpGs2200::connect()
{
	ATCMD_RESP_E resp;
	ATCMD_NetworkStatus networkStatus;

	resp = ATCMD_RESP_UNMATCH;
	WiFi_InitESCBuffer();
	do {
		resp = AtCmd_HTTPOPEN(&mCid, mData.host, mData.port);
		if (resp != ATCMD_RESP_OK) {
			ConsoleLog( "No Connect!" );
			delay(2000);
			continue;
		}
		if (mCid == ATCMD_INVALID_CID) {
			ConsoleLog( "No CID!" );
			delay(2000);
			continue;
		}
	} while (ATCMD_RESP_OK != resp);

	do {
		resp = AtCmd_NSTAT(&networkStatus);
	} while (ATCMD_RESP_OK != resp);

	ConsoleLog( "Connected" );
	ConsolePrintf("IP: %d.%d.%d.%d\r\n", 
	              networkStatus.addr.ipv4[0], networkStatus.addr.ipv4[1], networkStatus.addr.ipv4[2], networkStatus.addr.ipv4[3]);
	return true;
}

bool HttpGs2200::send(ATCMD_HTTP_METHOD_E type, uint8_t timeout, const char *page, const char *msg, uint32_t size)
{
	ATCMD_RESP_E resp = ATCMD_RESP_UNMATCH;
	bool result = false;

	resp = AtCmd_HTTPSEND(mCid, type, timeout, page, msg, size);
  if (ATCMD_RESP_OK == resp || ATCMD_RESP_BULK_DATA_RX == resp) {
		result = true;
  } else {
		result = false;
	}
	return result;
}

int HttpGs2200::receive(uint8_t* data, int length)
{
	int receive_size = -1;

	if (mWifi->available()) {
		receive_size = mWifi->read(mCid, data, length);
		if (0 < receive_size) {
			memset(data, 0, length);
			WiFi_InitESCBuffer();
		}
	}

  return receive_size;
}

bool HttpGs2200::receive()
{
	ATCMD_RESP_E resp = ATCMD_RESP_UNMATCH;
	bool result = false;

	if (mWifi->available()) {
		resp = AtCmd_RecvResponse();
		if (ATCMD_RESP_OK == resp) {
			result = true;
		} else {
			result = false;
		}
	}
  return result;
}

bool HttpGs2200::end()
{
	ATCMD_RESP_E resp = ATCMD_RESP_UNMATCH;
	bool result = false;

	resp = AtCmd_HTTPCLOSE(mCid);

	if (ATCMD_RESP_OK == resp || ATCMD_RESP_INVALID_CID == resp ) {
		result = true;
	} else {
		result = false;
	}

  return result;
}
