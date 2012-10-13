/* include udpserv1 */
#include "unpifi.h"

void	mydg_echo(int, SA *, socklen_t, SA *);

int
main(int argc, char **argv)
{
	printf("\nServer Program \n");
	int sockfd;
	const int on = 1;
	pid_t	child_pid;
	struct ifi_info		*ifi, *ifihead;
	struct sockaddr_in	*sa, cliaddr;  //TODO Why pointer for one alone?
	int server_port;
	int window_size;
	
	//Read server.in file
	//TODO Error check here for wrong values input from user
	FILE *fr = fopen ("server.in", "r");  
	
	fscanf (fr, "%d", &server_port);
	fscanf (fr, "%d", &window_size);
	fclose(fr);
	
	printf("\nServer Port : %d\n", &server_port);
	printf("\nWindow Size : %d\n", &window_size);
	
	//TODO How to make sure that only unicast addresses are bound??	
	
	//Iterating through all the interfaces
	for (ifihead = ifi = Get_ifi_info(AF_INET, 1);
		 ifi != NULL; ifi = ifi->ifi_next) 
	{

		sockfd = socket(AF_INET, SOCK_DGRAM, 0);

		setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

		sa = (struct sockaddr_in *) ifi->ifi_addr;
		sa->sin_family = AF_INET;
		sa->sin_port = htons(10987);
		bind(sockfd, (SA *) sa, sizeof(*sa));
		printf("Bound %s\n", sock_ntop((SA *) sa, sizeof(*sa)));	
	}
	exit(0);
}

//SPARE CODE

/*
		if( (child_pid = fork()) != -1)
		{
			if(child_pid == 0)
			{
				//In Child
				mydg_echo(sockfd, (SA *) &cliaddr, sizeof(cliaddr), (SA *) sa);
			}
			else
			{
				//In Server
				printf("Child (PID: %d) forked for the interface %s", child_pid,
					sock_ntop(sa, sizeof(sa)));
				
			}
		}
		*/

/*
void
mydg_echo(int sockfd, SA *pcliaddr, socklen_t clilen, SA *myaddr)
{
	int			n;
	char		mesg[MAXLINE];
	socklen_t	len;

	for ( ; ; ) {
		len = clilen;
		n = recvfrom(sockfd, mesg, MAXLINE, 0, pcliaddr, &len);
		printf("child %d, datagram from %s", getpid(),
			   Sock_ntop(pcliaddr, len));
		printf(", to %s\n", Sock_ntop(myaddr, clilen));

		sendto(sockfd, mesg, n, 0, pcliaddr, len);
	}
}
/* end mydg_echo */
