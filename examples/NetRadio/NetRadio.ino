/*
 *  SPRESENSE_WiFi.ino - GainSpan WiFi Module Control Program
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

#include <TelitWiFi.h>
#include "config.h"
#include <Audio.h>
#include <LowPower.h>

#define  CONSOLE_BAUDRATE  115200

/*-------------------------------------------------------------------------*
 * Globals:
 *-------------------------------------------------------------------------*/
AudioClass *theAudio;
TelitWiFi gs2200;
TWIFI_Params gsparams;
char server_cid = 0;
char TCP_Data[]=RADIO_SITE;
const uint16_t TCP_RECEIVE_PACKET_SIZE = 1500;
uint8_t TCP_Receive_Data[TCP_RECEIVE_PACKET_SIZE] = {0};

enum State {
	E_RadioStart,
	E_AudioStart,
	E_Run,
	E_Stop,
	StateNum
};

State state = E_RadioStart;

/**------------------------------------------------------------------------
 * @brief Audio attention callback
 *
 * When audio internal error occurc, this function will be called back.
 */

static void audio_attention_cb(const ErrorAttentionParam *atprm)
{
	puts("Attention!");
  
	//if (atprm->error_code > AS_ATTENTION_CODE_WARNING)
	if (atprm->error_att_sub_code == AS_ATTENTION_SUB_CODE_SIMPLE_FIFO_UNDERFLOW) {
		state = E_Stop;
		digitalWrite( LED2, HIGH ); // turn off LED
		digitalWrite( LED3, HIGH ); // turn on LED
	} else if (atprm->error_att_sub_code == AS_ATTENTION_SUB_CODE_DSP_EXEC_ERROR) {
		// Don't Care.
	} else {
		LowPower.reboot();
	}
}

/**------------------------------------------------------------------------
 * the setup function runs once when you press reset or power the board
 */
void setup() {

	LowPower.begin();

	/* initialize digital pin LED_BUILTIN as an output. */
	pinMode(LED0, OUTPUT);
	digitalWrite( LED0, LOW );   // turn the LED off (LOW is the voltage level)
	Serial.begin( CONSOLE_BAUDRATE ); // talk to PC

	/* Initialize AT Command Library Buffer */
	AtCmd_Init();
	/* Initialize SPI access of GS2200 */
	Init_GS2200_SPI_type(iS110B_TypeC);

	digitalWrite( LED0, HIGH ); // turn on LED
  
	// start audio system
	theAudio = AudioClass::getInstance();
	theAudio->begin(audio_attention_cb);
	puts("initialization Audio Library");
	theAudio->setPlayerMode(AS_SETPLAYER_OUTPUTDEVICE_SPHP, AS_SP_DRV_MODE_4DRIVER);
	err_t err = theAudio->initPlayer(AudioClass::Player0, AS_CODECTYPE_MP3, "/mnt/sd0/BIN", AS_SAMPLINGRATE_AUTO, AS_CHANNEL_STEREO);
	/* Verify player initialize */
  if (err != AUDIOLIB_ECODE_OK) {
    printf("Player0 initialize error\n");
    exit(1);
  }
	puts("initialization Player Library");
	
	theAudio->setVolume(-160);
	
	/* Initialize AT Command Library Buffer */
	gsparams.mode = ATCMD_MODE_STATION;
	gsparams.psave = ATCMD_PSAVE_ALWAYS_ON;
	if (gs2200.begin(gsparams)) {
		ConsoleLog("GS2200 Initilization Fails");
		while(1);
	}

	/* GS2200 Association to AP */
	if (gs2200.activate_station(AP_SSID, PASSPHRASE)) {
		ConsoleLog("Association Fails");
		while(1);
	}
	
	digitalWrite( LED0, LOW );  // turn off LED
	digitalWrite( LED1, HIGH ); // turn on LED
}

/**------------------------------------------------------------------------
 * Start process for web radio
 */
void start_radio() {
	do {
		// Start a TCP client
		server_cid = gs2200.connect(RADIO_IP, RADIO_PORT);
	} while (server_cid == ATCMD_INVALID_CID);
	digitalWrite( LED1, LOW );  // turn off LED
	digitalWrite( LED2, HIGH ); // turn on LED
}

/**------------------------------------------------------------------------
 * Write to AudioSubSytem @ RadioStart State
 */
void write_StartRadio(uint8_t* pt, uint32_t sz)
{
	while (sz > 3) {
		if (('\r' == *(pt)) &&
		    ('\n' == *(pt+1)) &&
		    ('\r' == *(pt+2)) &&
		    ('\n' == *(pt+3))) {
			state = E_AudioStart;
			pt = pt+3;
			sz = sz-3;
			ConsoleLog( "\nfind\n");
			break;
		}
		pt++;
		sz--;
	}
	if (state == E_AudioStart) {
		write_StartAudio(pt, sz);
	}
}

/**------------------------------------------------------------------------
 * Write to AudioSubSytem @ AudioStart State
 */
const int start_size = 8000;
void write_StartAudio(uint8_t* pt, uint32_t sz)
{
	static int buffered_size = 0;
	
	buffered_size += sz;
	if (buffered_size > start_size) {
		puts("start");
		theAudio->startPlayer(AudioClass::Player0);
		state = E_Run;
		buffered_size = 0;
		digitalWrite( LED2, LOW );  // turn off LED
		digitalWrite( LED3, HIGH ); // turn on LED
	}
	write_Run(pt,sz);
}

/**------------------------------------------------------------------------
 * Write to AudioSubSytem @ Runnig State
 */
void write_Run(uint8_t* pt, uint32_t sz)
{
	err_t err = AUDIOLIB_ECODE_SIMPLEFIFO_ERROR;
	while (err == AUDIOLIB_ECODE_SIMPLEFIFO_ERROR) {
		err = theAudio->writeFrames(AudioClass::Player0, pt, sz);
		usleep(10000);
	}
}

/**------------------------------------------------------------------------
 * ES reader thread
 */
static int es_reader(int argc, FAR char *argv[])
{
	(void)argc;
	(void)argv;
	int receive_size = 0;

	while (1) {
		/*Wait for data.*/
		for (int i= 200; gs2200.available() == 0; i--) {
			if (i==0) {
			  LowPower.reboot();
			}
			usleep(10000);
		}
		
		while (gs2200.available()) {
			receive_size = gs2200.read(server_cid, TCP_Receive_Data, TCP_RECEIVE_PACKET_SIZE);
			if (0 < receive_size) {
				switch (state) {
				case E_RadioStart:
					write_StartRadio(TCP_Receive_Data, receive_size);
					break;
				case E_AudioStart:
					write_StartAudio(TCP_Receive_Data, receive_size);
					break;
				case E_Run:
					write_Run(TCP_Receive_Data, receive_size);
					break;
				case E_Stop:
					theAudio->stopPlayer(AudioClass::Player0,AS_STOPPLAYER_NORMAL);
					state = E_AudioStart;
					digitalWrite( LED3, LOW );  // turn off LED
					digitalWrite( LED2, HIGH ); // turn on LED
					break;
				default:
					puts("error!");
					exit(1);
				}
				WiFi_InitESCBuffer();
			}
		}
	}
}

/**------------------------------------------------------------------------
 * the loop function runs over and over again forever
 */

void loop() {

	start_radio();

	// Connect Net Radio server
  while (1) {
    if (true != gs2200.write(server_cid, (const uint8_t*)TCP_Data, strlen(TCP_Data))) {
      // Data is not sent, we need to re-send the data
      delay(1);
      continue;
    } else {
      break;
    }
  }
	puts(RADIO_NAME);
	task_create("es_reader", 155, 1024, es_reader, NULL);

	while (1) {
		sleep(10000);
	}
}
