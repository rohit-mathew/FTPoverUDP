/* include udpserv1 */
#include "unpifi.h"

void	mydg_echo(int, SA *, socklen_t, SA *);

struct socket_info{
	int sock_fd;
	uint32_t ipaddress;
	uint32_t network_mask;
	uint32_t subnet_address;
 	};

int main(int argc, char **argv)
{
	printf("\nServer Program \n");
	int sockfd;
	const int on = 1;
	pid_t	child_pid;
	struct ifi_info		*ifi, *ifihead;
	struct sockaddr_in	*sa; 
	int server_port;
	int window_size;
	struct socket_info intf_info[128];
	
	//Read server.in file
	//TODO Error check here for wrong values input from user
	FILE *fr = fopen ("server.in", "r");  
	
	if(fr == NULL)
	{
		printf("Unable to open file server.in. Please check if it is in this directory\n");
		exit(0);
	}
	
	fscanf (fr, "%d", &server_port); 
	fscanf (fr, "%d", &window_size); 

	fclose(fr);
	
	printf("\nServer Port : %d", server_port);
	printf("\nWindow Size : %d\n", window_size);
	
	//TODO How to make sure that only unicast addresses are bound??	
	
	//Iterating through all the interfaces
	int noofintf;
	for (ifihead = ifi = Get_ifi_info(AF_INET, 1);
		 ifi != NULL; ifi = ifi->ifi_next) 
	{

		sockfd = socket(AF_INET, SOCK_DGRAM, 0);

		setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)); //TODO Needed???

		sa = (struct sockaddr_in *) ifi->ifi_addr;
		sa->sin_family = AF_INET;
		sa->sin_port = htons(server_port);
		bind(sockfd, (SA *) sa, sizeof(*sa));
		printf("\n Bound %s\n", sock_ntop((SA *) sa, sizeof(*sa)));
		intf_info[noofintf].sock_fd = sockfd;
		intf_info[noofintf].ipaddress = sa->sin_addr.s_addr;
		intf_info[noofintf].network_mask = 0;
		intf_info[noofintf].subnet_address = 0;
		noofintf = noofintf+1;
	}
	noofintf = noofintf-1;
	char str[128];
	inet_ntop(AF_INET, &(intf_info[1].ipaddress), str, INET_ADDRSTRLEN);
	printf("Ip address is %s", str);
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
