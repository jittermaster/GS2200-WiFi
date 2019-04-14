/*
 *  config.h - WiFi Configration Header
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

#ifndef _CONFIG_H_
#define _CONFIG_H_

/*-------------------------------------------------------------------------*
 * Configration
 *-------------------------------------------------------------------------*/
#define  AP_SSID     "SSID_AP"
#define  PASSPHRASE  "1234567890"

//#define JPOP_SAKURA
#define DANCE_WAVE
//#define DANCE_WAVE_RETRO

#ifdef JPOP_SAKURA
#define  RADIO_NAME     "J-Pop Sakura"
#define  RADIO_FILENAME "stream"
#define  RADIO_IP       "158.69.38.195"
#define  RADIO_PORT     "20278"
#endif

#ifdef DANCE_WAVE
#define  RADIO_NAME     "Dance Wave!"
#define  RADIO_FILENAME "dance.mp3"
#define  RADIO_IP       "dancewave.online"
#define  RADIO_PORT     "8080"
#endif

#ifdef DANCE_WAVE_RETRO
#define  RADIO_NAME     "Dance Wave Retro!"
#define  RADIO_FILENAME "retrodance.mp3"
#define  RADIO_IP       "dancewave.online"
#define  RADIO_PORT     "8080"
#endif

#define  RADIO_SITE "GET /" RADIO_FILENAME " HTTP/1.1\r\nHOST: " RADIO_IP ":" RADIO_PORT "\r\nConnection: close\r\n\r\n"

#endif /*_CONFIG_H_*/
