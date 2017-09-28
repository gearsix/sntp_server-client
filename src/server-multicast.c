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
	char* bcastaddress = "226.1.1.1";
	char* bcastinterface;
	struct ip_mreq group;
	struct sockaddr_in server;
	struct sntp_packet buffer;
	socklen_t addresslen = sizeof(struct sockaddr);
	size_t bufferlen = sizeof(struct sntp_packet);
	
//operations
	printTitle("SNTP-server_multicast");
	//check arguments for broadcast interface to create broadcast address on & any specified port
	printf ("Checking arguments..\n");
	switch(argc)
	{	case 1:
		{	fperror("\tNo arguments found; \"./server-multicast <broadcast interface> <port> <broadcast address>\"\n");
			exit(1);
			break;
		}
		case 2:
		{	bcastinterface = argv[1];
			printf ("\tBroadcast interface found; using \"%s\"\n", bcastinterface);
			printf ("\tNo port found; using default port\n");
			printf ("\tDefault broadcast address: %s\n", bcastaddress);
			break;
		}
		case 3:
		{	bcastinterface = argv[1];
			printf ("\tBroadcast interface found; using  \"%s\"\n", bcastinterface);
			port = atoi(argv[2]);
			printf ("\tPort found; using \"%i\"\n", port);
			printf ("\tDefault broadcast address: %s\n", bcastaddress);
			break;
		}
		case 4:
		{	bcastinterface = argv[1];
			printf ("\tBroadcast interface found; using \"%s\"\n", bcastinterface);
			port = atoi(argv[2]);
			printf ("\tPort found; using \"%i\"\n", port);
			bcastaddress = argv[3];
			printf ("\tBroadcast address: %s\n", bcastaddress);
			break;
		}
		default:
		{	fperror("\tToo many arguments found; \"./server-multicast  <broadcast interface> <port>\"");
			exit(1);
			break;
		}
	}

	//create socket
	printf ("Creating socket..\t");
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{	fperror("socket() failure");
		exit(1);
	}
	puts ("Success!");
	 

	
	//join multicast group
	printf ("Defining multicast details..\n");
	group.imr_multiaddr.s_addr = inet_addr(bcastaddress);
	group.imr_interface.s_addr = inet_addr(bcastinterface);
	printf ("Setting more socket options..\t");
	puts ("Success!");
	//set socket option
	printf ("Setting socket options..\t");
		//allow multiple sockets to use this port
	i = 1;
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&i, sizeof(i)) < 0)
	{	fflush(stdout);
		fperror("setsockopt() failure");
		close(sock);
		exit(1);
	}
		//Notify kernel we're intrested in multicast packets
	if(setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &group, sizeof(group)) < 0)
	{	fflush(stdout);
		perror("setsockopt() failed");
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
	{	fflush(stdout);
		perror("bind() failed");//fperror("bind() failed\n");
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
