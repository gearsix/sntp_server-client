/*===========
 |	MARCOS	|
 ===========*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>		//close(), sleep()
#include <string.h>		//strcmp()
#include <sys/time.h>	//gettimeofday()
#include <sys/types.h>	//size_t
#include <sys/socket.h>	//sockaddr_in, socklen_t, etc
#include <netdb.h>		//gethostbyname()
#include "libs/sntp-lib.h"	//struct sntp_packet, printTitle(), printUnixTime(), getNTPtimeofday(), convertNTPtoUNIX(), htonSNTPpacket(), ntohSNTPpacket()

#define POLL_INTERVAL 15	//seconds between sending request to server

#define DEFAULTHOSTCOUNT 5
#define DEFAULTHOST0 "ntp.uwe.ac.uk"
#define DEFAULTHOST1 "0.uk.pool.ntp.org"
#define DEFAULTHOST2 "1.uk.pool.ntp.org"
#define DEFAULTHOST3 "2.uk.pool.ntp.org"
#define DEFAULTHOST4 "3.uk.pool.ntp.org"

/*===========
 |	GLOBALS	|
 ===========*/
char* _defaulthost[DEFAULTHOSTCOUNT] = {DEFAULTHOST0, DEFAULTHOST1, DEFAULTHOST2, DEFAULTHOST3, DEFAULTHOST4};

struct timeval systemClock;

void initialiseRequest(struct sntp_packet *);
void replyRequest(struct sntp_packet *);
int validatePacket(struct sntp_packet);

/*===========
 |	 MAIN	|
 ===========*/
int main(int argc, char* argv[])
{
//variables
	int i;
	bool defaults;
	bool running;
	
	int sock;
	int port;
	char* hostname;
	struct hostent *host;
	struct sockaddr_in server;
	struct sntp_packet buffer;
	socklen_t addresslen = sizeof(struct sockaddr);
	size_t bufferlen = sizeof(struct sntp_packet);
	
//operations
	printTitle("SNTP_client-unicast");
	
	//check arguments for specified host & port
	printf ("Checking arguments..\n");
	switch(argc)
	{	case 1:
		{	defaults = true;
			printf ("\tNo arguments found; using default values\n");
			port = NTPPORT;
			break;
		}
		case 2:
		{	defaults = false;
			hostname = argv[1];
			printf ("\tHost found; using \"%s\"\n", hostname);
			port = LOCALPORT; //assuming specified host is my own server
			printf ("\tNo port found; using default port (%d)\n", port);
			
			break;
		}
		case 3:
		{	defaults = false;
			hostname = argv[1];
			printf ("\tHost found; using \"%s\"\n", hostname);
			port = atoi(argv[2]);
			printf ("\tPort found; using \"%i\"\n", port);
			break;
		}
		default:
		{	fprintf(stderr, "\tToo many arguments found; \"./c <host address> <port>\"\n");
			exit(1);
			break;
		}
	}
	
	//resolve host
	printf ("Resolving host..\n");
	if (defaults)
	{	for (i = 0; i < DEFAULTHOSTCOUNT; i++)
		{	printf ("\tdefaulthost%i(%s)..\t", i, _defaulthost[i]);
			host = gethostbyname(_defaulthost[i]);
			if (host == NULL)
			{	fflush(stdout);
				perror("gethostbyname() failure");
				if (i == DEFAULTHOSTCOUNT)
					exit(1);
			}
			else
				i = DEFAULTHOSTCOUNT;				
		}
	}
	else
	{	printf ("\t%s..\t", argv[1]);
		if ((host = gethostbyname(argv[1])) == NULL)
		{	fflush(stdout);
			perror("gethostbyname() failure");
			exit(1);
		}
	}
	puts ("Success!");
	
	//define server
	printf ("Defining server details..\n");
	memset(&server, 0, sizeof(server));	//zero all server values
	server.sin_family = AF_INET;							//host byte order
	server.sin_port = htons(port);							//network byte order (short)
	server.sin_addr = *((struct in_addr *)host->h_addr);	//host's address
	
	
	//create socket
	printf ("Creating socket..\t");
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{	fflush(stdout);
		perror("socket() failed");
		exit(1);
	}	
	puts ("Success!");
	
	puts ("READY!");
	printf ("Initial System Clock: ");
	printUnixTime(systemClock);
	
	//main loop
	running = true;
	while (running)
	{	//initialise request
		printf ("\nInitialising request..\n");
		initialiseRequest(&buffer);
		
		//send request
		printf ("Sending request..\t");
		if (sendto(sock, &buffer, bufferlen, 0, (struct sockaddr *)&server, addresslen) == -1)
		{	fflush(stdout);
			perror("sendto() failed");
			close(sock);
			exit(1);
		}
		puts ("\tRequest sent");
		
		//recieve reply
		printf ("Listening for reply..\t");
		if (recvfrom(sock, &buffer, bufferlen, 0, (struct sockaddr *)&server, &addresslen) == -1)
		{	fflush(stdout);
			perror ("error: recvfrom() failed\n");
			close(sock);
			exit(1);
		}
		puts ("\tReply recieved");
		
		//process reply
		printf ("Processing reply..\n");
		ntohSNTPpacket(&buffer);
		
		//check recieved packet for errors
		printf ("Checking packet for errors..\t");
		i = validatePacket(buffer);
		if (i == 0)
		{	puts ("Packet valid");
			
			//update system clock
			printf ("Updating system clock..\n");
			convertNTPtoUNIX(buffer.transmit_ts, &systemClock);
		}
		else
		{	printf ("Packet invalid (%i)\n", i);
			//official client would resolve new host & define new server here
			running = false;	//exit loop instead of sleeping because it probably means there's something wrong with my request
		}
		
		//print intial system clock
		printf ("Current System Clock: ");
		printUnixTime(systemClock);
		
		//wait POLL_INTERVAL before sending another request
		printf ("Sleeping for %i...\n", POLL_INTERVAL);
		sleep(POLL_INTERVAL);
	}
	
	printf ("\nclosing socket..\n");
	close (sock);
	printf ("closing client..\n");
	
	return 0;	
}

/*===============
 |	 FUNCTIONS	|
 ===============*/
//prepare buffer (sntp_packet buffer) as sntp request
void initialiseRequest(struct sntp_packet *request)
{	
	memset(request, 0, sizeof(&request)); 		//clear the buffer
	request->flags = 0x23; 						//set flags	//li=0_vn=4_mode=3 (0x23 = 00_100_011)
	request->transmit_ts = getNTPtimeofday();	//set transmit timestamp to time of day
	htonSNTPpacket(request);
}

//checks the recieved sntp_packet for errors (valid packet = true)
int validatePacket(struct sntp_packet reply)
{
	//sanity check 4
	if (reply.flags != 0x24)
	{	printf ("%x", reply.flags);	
		return 40;
	}
	if (reply.stratum == 0)
		return 41;
	if (reply.transmit_ts.seconds == 0 && reply.transmit_ts.fraction == 0)
		return 42;
		
	//sanity check 5
	if (reply.rootDelay < 0 || reply.rootDispersion < 0)
		return 5;

	return 0;
}
