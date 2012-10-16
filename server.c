/* include udpserv1 */
#include "unpifi.h"

void	mydg_echo(int);

struct socket_info{
	int sock_fd;
	uint32_t ipaddress;
	uint32_t network_mask;
	uint32_t subnet_address;
 	};

int main(int argc, char **argv)
{
	printf("\nServer Program \n");
	int maxfdp1 = 0; 
	fd_set rset;
	
	int sockfd;
	int i; 
	const int on = 1;
	pid_t	child_pid;
	struct ifi_info	*ifi, *ifihead;
	struct sockaddr_in *sa;
	
	int server_port;
	int window_size;
	
	struct socket_info intf_info[128];
	
	//Read server.in file
	//TODO Error check here for wrong values input from user
	FILE *fr = fopen ("server.in", "r");  
	
	if(fr == NULL)
	{
		printf("Unable to open file server.in. Please check. \n");
		exit(0);
	}
	
	fscanf (fr, "%d", &server_port); 
	fscanf (fr, "%d", &window_size); 

	fclose(fr);
	
	printf("\nDetails from server.in file :-\n");
	printf("\nServer Port : %d", server_port);
	printf("\nWindow Size : %d\n", window_size);
	
	//TODO How to make sure that only unicast addresses are bound??	
	
	//Iterating through all the interfaces
	int noofintf;
	for (ifihead = ifi = Get_ifi_info_plus(AF_INET, 1);
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
	/*
	char str[128];
	inet_ntop(AF_INET, &(intf_info[1].ipaddress), str, INET_ADDRSTRLEN);
	printf("Ip address is %s\n", str);
	*/
	
	for(i=0; i<=noofintf ; i++)
	{
		if(intf_info[i].sock_fd > maxfdp1)
			maxfdp1 = intf_info[i].sock_fd;
	}
	
	while(1)
	{
		FD_ZERO(&rset);
		for(i=0; i<=noofintf ; i++)
		{
			FD_SET(intf_info[i].sock_fd, &rset);
		}
		
		int k = select(maxfdp1, &rset, NULL, NULL, NULL);
		
		for(i=0; i<=noofintf ; i++)
		{
			if(FD_ISSET(intf_info[i].sock_fd, &rset))
			{
				if( (child_pid = fork()) != -1)
				{
					if(child_pid == 0)
					{
						//In Child
						mydg_echo(intf_info[i].sock_fd);
					}
					else
					{
						printf("In the server after the fork");
						//In Server
						//printf("Child of(PID: %d) forked for the interface %s", child_pid,
						//	sock_ntop(sa, sizeof(sa)));
						
					}
				}
				
			}
		}
		
		
	}
	exit(0);
}


void mydg_echo(int sockfd)
{
	int n;
	char mesg[MAXLINE];
	struct sockaddr_in *pcliaddr;
	socklen_t len = sizeof(pcliaddr);

	for ( ; ; ) {
		n = recvfrom(sockfd, mesg, MAXLINE, 0, (SA*) pcliaddr, &len);
		//printf("child %d, datagram from %s", getpid(),
	//		   Sock_ntop(pcliaddr, len));
		//printf(", to %s\n", Sock_ntop(myaddr, clilen));

		sendto(sockfd, mesg, n, 0, pcliaddr, len);
	}
}

