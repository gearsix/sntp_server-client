/*===========
 |	MARCOS	|
 ===========*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>		//close(), sleep()
#include <sys/types.h>	//size_t
#include <sys/socket.h>	//sockaddr_in, socklen_t, etc
#include <netinet/in.h>	//htonl(), ntohl()
#include "libs/sntp-lib.h"	//struct sntp_packet, printTitle(), fperror(), getNTPtimeofday(), htonSNTPpacket(), ntohSNTPpacket()

/*===========
 |	GLOBALS	|
 ===========*/
bool validateRequest(struct sntp_packet);
void initialiseReply(struct sntp_packet *);

/*===========
 |	 MAIN	|
 ===========*/
int main(int argc, char* argv[])
{
//variables
	int sock;
	int port = LOCALPORT;
	struct sockaddr_in server;
	//struct sockaddr_in client;
	struct sntp_packet buffer;
	socklen_t addresslen = sizeof(struct sockaddr);
	size_t bufferlen = sizeof(struct sntp_packet);
	
	printTitle("SNTP-server_unicast");
//create socket
	printf ("Creating socket..\t\t");
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		fperror("socket() failure");
	puts ("Success!");
	
//define server details
	printf ("Defining server details..\t");
	memset(&server, 0, addresslen);		//zero all server values
	server.sin_family = AF_INET;		//host byte order
	server.sin_port = htons(port);		//network byte order (short)
	server.sin_addr.s_addr = INADDR_ANY;
	puts ("Success!");
	
//bind server
	printf ("Binding server..\t\t");
	if (bind(sock, (struct sockaddr *)&server, addresslen) == -1)
		fperror("bind() failed\n");
	puts ("Success!");
	
//main loop
	bool running = true;
	while (running)
	{	
	//clear the buffer
		memset(&buffer, 0, sizeof(buffer));
		
	//listen for requests
		printf ("\nListening for requests..\n");
		if (recvfrom(sock, &buffer, bufferlen, 0, (struct sockaddr *)&server, &addresslen) == -1)
		{	fperror ("recvfrom() failed\n");
			printf ("\nclosing socket..\n");
			close(sock);
			exit(EXIT_FAILURE);
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
				printf ("\nclosing socket..\n");
				close(sock);
				exit(EXIT_FAILURE);
			}
			puts ("\tReply sent");
		}
		else
			puts ("Request invalid, discarding");
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
{									//li|ver|mode
	if (request.flags == 0x23 ||	//00|100|011
		request.flags == 0x1B ||	//00|011|011
		request.flags == 0x13 ||	//00|010|011
		request.flags == 0x0B)		//00|001|011
		return true;
	else
		return false;
}

//initialises an sntp server reply packet, exepcts valid request
void initialiseReply(struct sntp_packet *reply)
{
	reply->recieve_ts = getNTPtimeofday();		//set time the request was recieved
	reply->flags = 0x24;
	reply->stratum = 2;							//server uses system clock which is syncronised by external source
	//using same poll (leave unchanged)
	reply->precision = 0;						//?
	reply->rootDelay = 0;
	reply->rootDispersion = 0;
	reply->referenceId = 0;						//not significant in stratum 2
	reply->reference_ts.seconds = 0;			//not significant in stratum 2
	reply->reference_ts.fraction = 0;
	reply->originate_ts = reply->transmit_ts;	//time request was sent
	reply->transmit_ts = getNTPtimeofday();		//set time reply is being sent
	
	htonSNTPpacket(reply);
}
