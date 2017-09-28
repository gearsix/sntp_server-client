#ifndef ALEX_SNTP_GENERIC_LIB
#define ALEX_SNTP_GENERIC_LIB
/*===========
|	MARCOS	|
===========*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>		//strlen()
#include <stdint.h>		//uint8_t, int8_t, uint32_t
#include <time.h>		//localtime(), strftime()
#include <sys/time.h>	//gettimeofday()
#include <netinet/in.h>	//htonl(), ntohl()

#define NTPPORT			123		//used as default
#define LOCALPORT		7154	//71 = G	//54 = 6	//G6, gotta leave my mark like.

#define EPOCH_OFFSET	0x83AA7E80	//70 years in seconds (01/01/1900 to 01/01/1970)

/*===========
 |	GLOBALS	|
 ===========*/
//ntp timestamp format (8 bytes)
typedef struct
{	uint32_t seconds;
	uint32_t fraction;
} ntp_timestamp;

//sntp message format (48 bytes)
struct sntp_packet
{   uint8_t flags;	//li_vn_mode
	uint8_t stratum;
	uint8_t poll;
	int8_t	precision;
	uint32_t rootDelay;
	uint32_t rootDispersion;
	uint32_t referenceId;
	ntp_timestamp reference_ts;
	ntp_timestamp originate_ts;
	ntp_timestamp recieve_ts;
	ntp_timestamp transmit_ts;
};

/*===============
 |	 FUNCTIONS	|
 ===============*/
//prints a fancy 3-line title :)
void printTitle(char* title)
{	
	int i;
	
	for (i = 0; i < strlen(title); i++)
	{	putchar('-');
		if (i == strlen(title)-1)
			printf("\n");
	}
	
	puts (title);
	
	for (i = 0; i < strlen(title); i++)
	{	putchar('-');
		if (i == strlen(title)-1)
			printf("\n\n");
	}
}

//fflush(stdout), print error(message); flushing stdout before printing stderr avoids debug messages getting mixed up in the log
void fperror(char* message)
{	
	fflush(stdout);
	fprintf (stderr, "%s\n", message);
}

//converts time to readable timestamp and prints it
void printUnixTime(struct timeval time)
{	
	char timeBuffer[64];
	time_t rawtime = time.tv_sec;
	struct tm *unixtime = localtime(&rawtime);
	
	strftime(timeBuffer, sizeof(timeBuffer), "%d/%m/%Y %H:%M:%S", unixtime);
	printf ("%s.%06d\n", timeBuffer, (int)time.tv_usec);
}
	
//converts unixtime timestamp into *ntptime timestamp 
void convertUNIXtoNTP(struct timeval unixtime, ntp_timestamp *ntptime)
{	ntptime->seconds = unixtime.tv_sec + EPOCH_OFFSET;
	ntptime->fraction = (uint32_t)((double)(unixtime.tv_usec+1)*(double)(1LL << 32)*1.0e-6);
}

//converts ntptime timestamp into *unixtime timestamp
void convertNTPtoUNIX(ntp_timestamp ntptime, struct timeval *unixtime)
{	unixtime->tv_sec = ntptime.seconds - EPOCH_OFFSET;
	unixtime->tv_usec = (uint32_t)((double)ntptime.fraction*1.0e6 / (double)(1LL << 32));
}

//gets current time in unix format & converts it to NTP time
ntp_timestamp getNTPtimeofday()
{	struct timeval unixtime;
	ntp_timestamp ntptime;
	
	gettimeofday(&unixtime, NULL);
	convertUNIXtoNTP(unixtime, &ntptime);
	
	return ntptime;
}

//converts certain sntp_packet values from host to network format
void htonSNTPpacket (struct sntp_packet *packet)
{	packet->rootDelay 				= htonl(packet->rootDelay);
	packet->rootDispersion 			= htonl(packet->rootDispersion);
	packet->referenceId 			= htonl(packet->referenceId);
	packet->reference_ts.seconds 	= htonl(packet->reference_ts.seconds);
	packet->reference_ts.fraction 	= htonl(packet->reference_ts.fraction);
	packet->originate_ts.seconds 	= htonl(packet->originate_ts.seconds);
	packet->originate_ts.fraction 	= htonl(packet->originate_ts.fraction);
	packet->recieve_ts.seconds 		= htonl(packet->recieve_ts.seconds);
	packet->recieve_ts.fraction 	= htonl(packet->recieve_ts.fraction);
	packet->transmit_ts.seconds 	= htonl(packet->transmit_ts.seconds);
	packet->transmit_ts.fraction 	= htonl(packet->transmit_ts.fraction);
}

//converts certain sntp_packet values from network to host format
void ntohSNTPpacket (struct sntp_packet *packet)
{	packet->rootDelay 				= ntohl(packet->rootDelay);
	packet->rootDispersion 			= ntohl(packet->rootDispersion);
	packet->referenceId 			= ntohl(packet->referenceId);
	packet->reference_ts.seconds 	= ntohl(packet->reference_ts.seconds);
	packet->reference_ts.fraction 	= ntohl(packet->reference_ts.fraction);
	packet->originate_ts.seconds 	= ntohl(packet->originate_ts.seconds);
	packet->originate_ts.fraction 	= ntohl(packet->originate_ts.fraction);
	packet->recieve_ts.seconds 		= ntohl(packet->recieve_ts.seconds);
	packet->recieve_ts.fraction 	= ntohl(packet->recieve_ts.fraction);
	packet->transmit_ts.seconds 	= ntohl(packet->transmit_ts.seconds);
	packet->transmit_ts.fraction 	= ntohl(packet->transmit_ts.fraction);
}

#endif
