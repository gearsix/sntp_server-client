/*===========
 |	MARCOS	|
 ===========*/
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <netdb.h>
#include "libs/sntp-lib.h"

#define MULTICASTADDRESS "226.1.1.1"

/*===========
 |	GLOBALS	|
 ===========*/
bool validateRequest(struct sntp_packet);
void initialiseReply(struct sntp_packet *);

/*===========
 |	 MAIN	|
 ===========*/
int main(int argc, char *argv[])
{
//variables
	int i;
	bool running;
	
	int sock;
	int port = LOCALPORT;
	char* mcastaddress = MULTICASTADDRESS;
	char* mcastinterface = "10.0.2.15";
	struct ip_mreq group;
	struct sockaddr_in server;
	struct sntp_packet buffer;
	socklen_t addresslen = sizeof(struct sockaddr);
	size_t bufferlen = sizeof(struct sntp_packet);
	
//operations
	printTitle("SNTP-server_multicast");
/*	//check arguments for specified host & port

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
			printf ("\tHost found; using host \"%s\"\n", hostname);
			port = (strcmp(hostname, "localhost") == 0) ? LOCALPORT : NTPPORT;
			printf ("\tNo port found; using default port\n");
			
			break;
		}
		case 3:
		{	defaults = false;
			hostname = argv[1];
			printf ("\tHost found; using host \"%s\"\n", hostname);
			port = atoi(argv[2]);
			printf ("\tPort found; using port \"%i\"\n", port);
			break;
		}
		default:
		{	fprintf(stderr, "\tToo many arguments found; \"./u-client <host address> <port>\"\n");
			exit(1);
			break;
		}
	}
*/	
	
	//create socket
	printf ("Creating socket..\t");
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{	fperror("socket() failure");
		exit(1);
	}
	puts ("Success!");
	 
	//join multicast group
	printf ("Defining multicast details..\n");
	group.imr_multiaddr.s_addr = inet_addr(mcastaddress);
	group.imr_interface.s_addr = inet_addr(mcastinterface);
	printf ("Setting more socket options..\t");
	puts ("Success!");
	//set socket option
	printf ("Setting socket options..\t");
		//allow multiple sockets to use this port
	i = 1;
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&i, sizeof(i)) < 0)
	{	fperror("setsockopt() failure");
		close(sock);
		exit(1);
	}
		//Notify kernel we're intrested in multicast packets
	if(setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &group, sizeof(group)) < 0)
	{	perror("setsockopt() failed");
		close(sock);
		exit(1);
	}
	puts("Success");

	//define server
	printf ("Defining server details..\n");
	memset(&server, 0, addresslen);		//zero all server values
	server.sin_family = AF_INET;		//host byte order
	server.sin_port = htons(port);		//network byte order (short)
	server.sin_addr.s_addr = INADDR_ANY;
	
	//bind server
	printf ("Binding server..\t");
	if (bind(sock, (struct sockaddr *)&server, addresslen) == -1)
	{	perror("bind() failed");//fperror("bind() failed\n");
		close(sock);
		exit(1);
	}
	puts ("Success!");
	puts ("READY!");
	
	//main loop
	running = true;
	while (running)
	{	memset(&buffer, 0, sizeof(buffer));  //clear the buffer
		//listen for requests
		printf ("\nListening for requests..\n");
		if (recvfrom(sock, &buffer, bufferlen, 0, (struct sockaddr *)&server, &addresslen) == -1)
		{	fperror ("recvfrom() failed\n");
			close(sock);
			exit(1);
		}
		puts ("\tRequest recieved");
		
		//process request
		printf ("Processing request..\n");
		ntohSNTPpacket(&buffer);
		
		//check request is valid
		printf ("Validating request..\t");
		if (validateRequest(buffer) == true)
		{	puts ("Request valid");
	
			//initialise reply
			printf ("Initialising reply..\n");
			initialiseReply(&buffer);
			
			//send reply
			printf ("Sending reply..\n");
			if (sendto(sock, &buffer, bufferlen, 0, (struct sockaddr *)&server, addresslen) == -1)
			{	fperror("sendto(reply) failed");
				close(sock);
				exit(1);
			}
			puts ("\tReply sent");
		}
		else
			puts ("Request invalid");
	}
	
	printf ("\nclosing socket..\n");
	close (sock);
	printf ("closing client..\n");
	
	return 0;
}

/*===============
 |	 FUNCTIONS	|
 ===============*/
//checks that mode = 3 & version number = (1 - 4)
bool validateRequest(struct sntp_packet request)
{
	if (request.flags == 0x23 || request.flags == 0x1B || request.flags == 0x13 || request.flags == 0x0B)
		return true;
	else
		return false;
}

//checks the recieved sntp_packet for errors (valid request = true)
void initialiseReply(struct sntp_packet *reply)
{
	reply->recieve_ts = getNTPtimeofday();	//set time the request was recieved
	reply->flags = 0x24;
	reply->stratum = 2;			//server uses system clock which is syncronised by external source
	//use the same poll
	reply->precision = 0;		//?
	reply->rootDelay = 0;
	reply->rootDispersion = 0;
	reply->referenceId = 0;		//not significant in stratum 2
	reply->reference_ts.seconds = 0;	//not significant in stratum 2
	reply->reference_ts.fraction = 0;
	reply->originate_ts = reply->transmit_ts;	//time request was sent
	reply->transmit_ts = getNTPtimeofday();	//set time reply is being sent
	
	htonSNTPpacket(reply);
}
