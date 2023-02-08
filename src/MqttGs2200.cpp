/*
 *  MQTT Library for GS2200
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

#include "MqttGs2200.h"

// #define MQTT_DEBUG

#ifdef MQTT_DEBUG
#define DBG(...) { Serial.print(__VA_ARGS__); }
#define ERR(...) { Serial.print(__VA_ARGS__); }
#else
#define DBG(...)
#define ERR(...)
#endif /* MQTT_DEBUG */


bool MqttGs2200::begin(TelitWiFi* wifi, MQTTGS2200_HostParams* params)
{
#ifdef MQTT_DEBUG
    Serial.println("Initialize MQTT");
#endif

  mWifi = wifi;
  mCid = ATCMD_INVALID_CID;

  mData.clientID = params->clientID;
  mData.host = params->host;
  mData.port = params->port;
  mData.userName = params->userName;
  mData.password = params->password;
  
#ifdef MQTT_DEBUG
  Serial.println("Server: " + (String)mData.host + ", Port: " + mData.port);
  Serial.println("UserName: " + (String)mData.userName + ", Password: " + mData.password + ", clientID: " + mData.clientID);
#endif

  return true;
}

bool MqttGs2200::connect()
{
	ATCMD_RESP_E resp;
	ATCMD_NetworkStatus networkStatus;

	resp = ATCMD_RESP_UNMATCH;
	WiFi_InitESCBuffer();

	resp = AtCmd_MQTTCONNECT( &mCid, mData.host, mData.port, mData.clientID, mData.userName, mData.password);

	if (resp != ATCMD_RESP_OK) {
		ConsoleLog( "No Connect!" );
		delay(2000);
		return false;
	}

	if (mCid == ATCMD_INVALID_CID) {
		ConsoleLog( "No CID!" );
		delay(2000);
		return false;
	}

	do {
		resp = AtCmd_NSTAT(&networkStatus);
	} while (ATCMD_RESP_OK != resp);

	ConsoleLog( "Connected" );
	ConsolePrintf("IP: %d.%d.%d.%d\r\n", 
	              networkStatus.addr.ipv4[0], networkStatus.addr.ipv4[1], networkStatus.addr.ipv4[2], networkStatus.addr.ipv4[3]);
	return true;
}

bool MqttGs2200::publish(MQTTGS2200_Mqtt* mqtt)
{
  if (ATCMD_RESP_OK != AtCmd_MQTTPUBLISH(mCid, mqtt->params)) {
		return false;
  }

  mWifi->stop(mCid);
  return true;
}

bool MqttGs2200::subscribe(MQTTGS2200_Mqtt* mqtt)
{
	ATCMD_RESP_E resp = ATCMD_RESP_UNMATCH;
	bool result = false;

	resp = AtCmd_MQTTSUBSCRIBE(mCid, mqtt->params);
  if (ATCMD_RESP_OK == resp) {
		result = true;
	} else {
		result = false;
	}

  return result;
}

bool MqttGs2200::receive(String& data)
{
	ATCMD_RESP_E resp = ATCMD_RESP_UNMATCH;
	bool result = true;

	resp = AtCmd_RecieveMQTTData(data);
	if (ATCMD_RESP_DISCONNECT == resp) {
		result = false;
	} else {
		result = true;
	}

  return result;
}
