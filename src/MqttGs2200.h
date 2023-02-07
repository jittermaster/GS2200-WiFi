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

#ifndef MQTT_GS2200_h
#define MQTT_GS2200_h

#include <Arduino.h>
#include <GS2200Hal.h>
#include <GS2200AtCmd.h>
#include <TelitWiFi.h>


typedef struct {
  char *host;
  char *port;
  char *clientID;
  char *userName;
  char *password;
} MQTTGS2200_HostParams;

typedef struct {
  ATCMD_MQTTparams params;
} MQTTGS2200_Mqtt;

class MqttGs2200
{
public:

  MqttGs2200(){}
  ~MqttGs2200(){}

  bool begin(TelitWiFi* wifi, MQTTGS2200_HostParams* params);
  bool connect();
  bool send(MQTTGS2200_Mqtt* mqtt);
  void end(){ }

private:

  TelitWiFi* mWifi;
  char mCid;

  MQTTGS2200_HostParams mData;

};

#endif // MQTT_GS2200_h
