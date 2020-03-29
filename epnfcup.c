#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <regex.h>
#include <signal.h>

#include <nfc/nfc.h>

//#define DEBUG 1

// https://www.waveshare.com/wiki/4.2inch_NFC-Powered_e-Paper
// https://www.waveshare.com/wiki/ST25R3911B_NFC_Board
// look at ST25R3911B-NFC-Demo/epd-demo/User/Browser/Browser.c ;)

// Screen = 400*300 1 bit
// (300*400)/8 = buffer 15000 bytes

nfc_device *pnd;
nfc_context *context;

uint8_t idfromscreen[48];
uint8_t idextected[48] = {
	0x03, 0x27, 0xd4, 0x0f, 0x15, 0x61, 0x6e, 0x64, 0x72, 0x6f, 0x69, 0x64, 0x2e, 0x63, 0x6f, 0x6d,
	0x3a, 0x70, 0x6b, 0x67, 0x77, 0x61, 0x76, 0x65, 0x73, 0x68, 0x61, 0x72, 0x65, 0x2e, 0x66, 0x65,
	0x6e, 0x67, 0x2e, 0x6e, 0x66, 0x63, 0x74, 0x61, 0x67, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

uint8_t step0[]  = { 0xcd, 0x0d };			// ??
uint8_t step1[]  = { 0xcd, 0x00, 0x0a };	// select e-paper type and reset e-paper	4  : 2.13inch e-Paper
											//											7  : 2.9inch e-Paper
											//											10 : 4.2inch e-Paper
											//											14 : 7.5inch e-Paper
uint8_t step2[]  = { 0xcd, 0x01 };			// e-paper normal mode  type?
uint8_t step3[]  = { 0xcd, 0x02 };			// e-paper config1
uint8_t step4[]  = { 0xcd, 0x03 };			// e-paper power on
uint8_t step5[]  = { 0xcd, 0x05 };			// e-paper config2
uint8_t step6[]  = { 0xcd, 0x06 };			// EDP load to main
uint8_t step7[]  = { 0xcd, 0x07 };			// Data preparation
uint8_t step8[123]  = { 0xcd, 0x08, 0x64 };	// Data start command	2.13 inch(0x10:Send 16 data at a time)
											//						2.9  inch(0x10:Send 16 data at a time)
											//						4.2  inch(0x64:Send 100 data at a time)
											//						7.5  inch(0x78:Send 120 data at a time)
uint8_t step9[]  = { 0xcd, 0x18 };			// e-paper power on
uint8_t step10[] = { 0xcd, 0x09 };			// Refresh e-paper
uint8_t step11[] = { 0xcd, 0x0a };			// wait for ready
uint8_t step12[] = { 0xcd, 0x04 };			// e-paper power off command

static void sighandler(int sig)
{
	printf("Caught signal %d\n", sig);
	if (pnd != NULL) {
		nfc_abort_command(pnd);
		nfc_close(pnd);
	}
	nfc_exit(context);
	exit(EXIT_FAILURE);
}

int CardTransmit(nfc_device *pnd, uint8_t *capdu, size_t capdulen, uint8_t *rapdu, size_t *rapdulen)
{
	int res;

#ifdef DEBUG
	size_t  szPos;

	printf("=> ");
	for (szPos = 0; szPos < capdulen; szPos++) {
		printf("%02x ", capdu[szPos]);
	}
	printf("\n");
#endif

	if ((res = nfc_initiator_transceive_bytes(pnd, capdu, capdulen, rapdu, *rapdulen, 500)) < 0) {
		fprintf(stderr, "nfc_initiator_transceive_bytes error! %s\n", nfc_strerror(pnd));
		return -1;
	} else {
		*rapdulen = (size_t)res;
#ifdef DEBUG
		printf("<= ");
		for (szPos = 0; szPos < *rapdulen; szPos++) {
			printf("%02x ", rapdu[szPos]);
		}
		printf("\n");
#endif
		return 0;
	}
}

/*
 *	*capdu		: APDU to send
 *	capdulen	: APDU size
 *	rb0 & rb1	: expected r[0] and r[1]
 *	nretry		: number of try
 *	msec		: delay after each try
 */
int sendcmd(nfc_device *pnd, uint8_t *capdu, uint8_t capdulen, uint8_t rb0, uint8_t rb1, int nretry, int msec)
{
	uint8_t rx[20];
	size_t rxsz = sizeof(rx);
	int failnum = 0;

	// set something different from expected
	rx[0] = rb0+1;
	rx[1] = rb1+1;

	while(1) {
		CardTransmit(pnd, capdu, capdulen, rx, &rxsz);

		// Are reply bytes ok ?
		if(rx[0]==rb0 && rx[1]==rb1) {
			return(0);
		} else {
			failnum++;
			usleep(msec*1000);
			if(failnum > nretry) {
				return(-1);
			}
		}
	}
}

int main(int argc, const char *argv[])
{
	uint8_t step=0;
	size_t rxsz = 20;
	uint8_t rx[20];

	nfc_target nt;

	nfc_init(&context);
	if (context == NULL) {
		printf("Unable to init libnfc (malloc)\n");
		exit(EXIT_FAILURE);
	}

	if(signal(SIGINT, &sighandler) == SIG_ERR) {
		printf("Can't catch SIGINT\n");
		return(EXIT_FAILURE);
	}

	if(signal(SIGTERM, &sighandler) == SIG_ERR) {
		printf("Can't catch SIGTERM\n");
		return(EXIT_FAILURE);
	}

	const char *acLibnfcVersion = nfc_version();
	(void)argc;
	printf("%s uses libnfc %s\n", argv[0], acLibnfcVersion);

	pnd = nfc_open(context, NULL);
	if (pnd == NULL) {
		fprintf(stderr, "Error: %s\n", "Unable to open NFC device.");
		nfc_exit(context);
		exit(EXIT_FAILURE);
	}

	if (nfc_initiator_init(pnd) < 0) {
		nfc_perror(pnd, "nfc_initiator_init");
		exit(EXIT_FAILURE);
	}

	printf("NFC reader: %s opened\n", nfc_device_get_name(pnd));

	nfc_target ant[1];
	const nfc_modulation nmMifare = {
		.nmt = NMT_ISO14443A,
		.nbr = NBR_UNDEFINED, //NBR_106,
	};

	nfc_initiator_list_passive_targets(pnd,nmMifare,ant,1);
	printf("%s\n",nfc_strerror(pnd)); // print Success

	// Drop the field for a while
    nfc_device_set_property_bool(pnd, NP_ACTIVATE_FIELD, false);
	usleep(200*1000);

	nfc_device_set_property_bool(pnd, NP_EASY_FRAMING, false);
	nfc_device_set_property_bool(pnd, NP_HANDLE_PARITY, true);
	nfc_device_set_property_bool(pnd, NP_HANDLE_CRC, true);
	//nfc_device_set_property_bool(pnd, NP_ACCEPT_INVALID_FRAMES, true);
	nfc_device_set_property_bool(pnd, NP_INFINITE_SELECT, false);
	//nfc_device_set_property_int(pnd, NP_TIMEOUT_COM, 300);

	nfc_device_set_property_bool(pnd, NP_ACTIVATE_FIELD, false);

	// Poll for a ISO14443A (MIFARE) tag
	printf("Polling for target...\n");
	while (nfc_initiator_select_passive_target(pnd, nmMifare, NULL, 0, &nt) <= 0);
	printf("Target detected!\n");


	// From launch_app() in  MainActivity.java
	uint8_t test1[] = { 48,4 };
	uint8_t test2[] = { 48,8 };
	uint8_t test3[] = { 48,12 };
	size_t testsz = 2;
	uint8_t resp[16] = { 0 };
	size_t respsz = 16;

	CardTransmit(pnd, test1, testsz, resp, &respsz);
	memcpy(idfromscreen, resp, 16);
	usleep(100*1000);
	CardTransmit(pnd, test2, testsz, resp, &respsz);
	memcpy(idfromscreen+16, resp, 16);
	usleep(100*1000);
	CardTransmit(pnd, test3, testsz, resp, &respsz);
	memcpy(idfromscreen+32, resp, 16);
	usleep(100*1000);

	if(memcmp(idextected, idfromscreen, 48) != 0) {
		fprintf(stderr, "Wrong tag!\n");
		nfc_close(pnd);
		nfc_exit(context);
		exit(EXIT_SUCCESS);
	}

#if DEBUG
	printf("String = [%s]\n", idextected);
#endif

	rx[0]=1; rx[1]=1;

	while(1) {
		if(step==0) {
			printf("Step 0 : init ?\n");
			if(sendcmd(pnd, step0, 2, 0, 0, 10, 0) == 0)
				step = 1;
			else
				step = 14;
		} else if(step==1) {
			printf("Step 1 : select e-paper type and reset\n");
			if(sendcmd(pnd, step1, 3, 0, 0, 10, 0) == 0)
				step = 2;
			else
				step = 14;
			usleep(10*1000);
		} else if(step==2) {
			printf("Step 2 : e-paper normal mode\n");
			if(sendcmd(pnd, step2, 2, 0, 0, 50, 0) == 0)
				step = 3;
			else
				step = 14;
			usleep(100*1000);
		} else if(step==3) {
			printf("Step 3 : e-paper config1\n");
			if(sendcmd(pnd, step3, 2, 0, 0, 10, 0) == 0)
				step = 4;
			else
				step = 14;
			usleep(200*1000);
		} else if(step==4) {
			printf("Step 4 : e-paper power on\n");
			if(sendcmd(pnd, step4, 2, 0, 0, 10, 0) == 0)
				step = 5;
			else
				step = 14;
			usleep(500*1000);
		} else if(step==5) {
			printf("Step 5 : e-paper config2\n");
			if(sendcmd(pnd, step5, 2, 0, 0, 30, 0) == 0)
				step = 6;
			else
				step = 14;
			usleep(10*1000);
		} else if(step==6) {
			printf("Step 6 : EDP load to main\n");
			if(sendcmd(pnd, step6, 2, 0, 0, 10, 0) == 0)
				step = 7;
			else
				step = 14;
		} else if(step==7) {
			printf("Step 7 : Data preparation\n");
			if(sendcmd(pnd, step7, 2, 0, 0, 10, 0) == 0)
				step = 8;
			else
				step = 14;
		} else if(step==8) {
			printf("Step 8 : Data start command\n");
			for(int i = 0; i<150; i++) {
				rx[0]=1; rx[1]=1;
				// FIXME step8 + data
				CardTransmit(pnd, step8, 103, rx, &rxsz);
			}
			rx[0]=1; rx[1]=1;
			step=9;
		} else if(step==9) {
			printf("Step 9 : e-paper power on\n");
			if(sendcmd(pnd, step9, 2, 0, 0, 10, 0) == 0)
				step = 10;
			else
				step = 14;
		} else if(step==10) {
			printf("Step 10 : Refresh e-paper\n");
			if(sendcmd(pnd, step10, 2, 0, 0, 10, 0) == 0)
				step = 11;
			else
				step = 14;
			usleep(200*1000);
		} else if(step==11) {
			printf("Step 11 : wait for ready\n");
			if(sendcmd(pnd, step11, 2, 0xff, 0, 70, 100) == 0)
				step = 12;
			else
				step = 14;
		} else if(step==12) {
			printf("Step 12 : e-paper power off command\n");
			if(sendcmd(pnd, step12, 2, 0, 0, 1, 0) == 0)
				step = 13;
			else
				step = 14;
			usleep(200*1000);
		} else if(step==13) {
			printf("Step 13\n");
			step=14;
			printf("E-paper UPdate OK\n");
			usleep(200*1000);
		} else if(step==14) {
			printf("The End.\n");
			break;
		}
	}

	printf("NFC stop.\n");
	nfc_close(pnd);
	nfc_exit(context);
	exit(EXIT_SUCCESS);
}
