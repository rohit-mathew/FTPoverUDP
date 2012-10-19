/* include udpserv1 */
#include "unpifiplus.h"



struct socket_info{
	int sockfd;
	uint32_t ipaddress;
	uint32_t network_mask;
	uint32_t subnet_address;
 	};

void	mydg_echo(struct socket_info);

int main(int argc, char **argv)
{
	printf("\nServer Program \n");
	int maxfdp1 = 0; 
	fd_set rset;
	
	int sockfd;
	int i, result; 
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
		
		if( (bind(sockfd, (SA *) sa, sizeof(*sa))) == -1)
		{
			printf("\n server : Unable to bind socket : %d", sockfd);
			printf("\n server : The error number is as follows : %d. Exitting", errno);
			exit(0);
		}
		
		intf_info[noofintf].sockfd = sockfd;
		intf_info[noofintf].ipaddress = sa->sin_addr.s_addr;
		
		sa = (struct sockaddr_in *) ifi->ifi_ntmaddr;
		intf_info[noofintf].network_mask = sa->sin_addr.s_addr;
		
		intf_info[noofintf].subnet_address = 0;
		
		noofintf = noofintf+1;
	}
	
	//Sample code just to print values out
	
	char str[128];

	printf("\n There are %d interfaces.\n", noofintf);	
	noofintf = noofintf-1;	
	
	for(i=0; i<=noofintf ; i++)
	{
		printf("These are the details for interface no : %d\n", i);
		inet_ntop(AF_INET, &(intf_info[i].ipaddress), str, INET_ADDRSTRLEN);
		printf("IP Address is \t%s\n", str);
		inet_ntop(AF_INET, &(intf_info[i].network_mask), str, INET_ADDRSTRLEN);
		printf("Network Mask is %s\n", str);
		inet_ntop(AF_INET, &(intf_info[i].subnet_address), str, INET_ADDRSTRLEN);
		printf("Subnet Address is is %s\n", str);
		
		//Finding the maximum socket no
		if(intf_info[i].sockfd > maxfdp1)
			maxfdp1 = intf_info[i].sockfd;
	}
	
	maxfdp1 = maxfdp1 + 1;
	printf("Max : %d\n", maxfdp1);
	
	while(1)
	{		
		FD_ZERO(&rset);
		for(i=0; i<=noofintf ; i++)
		{
			FD_SET(intf_info[i].sockfd, &rset);
		}
		
		result = select(maxfdp1, &rset, NULL, NULL, NULL);
		printf("Result : %d\n", result);
		
		if(result == -1)
		{
			printf("\n Something wrong with select. Exitting. \n");
			exit(0);
		}

		printf("\n Reached here \n");
		for(i=0; i<=noofintf ; i++)
		{
			if(FD_ISSET(intf_info[i].sockfd, &rset))
			{
				
				if( (child_pid = fork()) != -1)
				{
					if(child_pid == 0)
					{
						//In Child
						//mydg_echo(intf_info[i]);
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


void mydg_echo(struct socket_info recvd_socket_info)
{
	int n;
	char mesg[MAXLINE];
	struct sockaddr_in *pcliaddr, *myaddr;
	socklen_t len = sizeof(pcliaddr);
	
	myaddr->sin_addr.s_addr = recvd_socket_info.ipaddress;

	for ( ; ; ) 
	{
		n = recvfrom(recvd_socket_info.sockfd, mesg, MAXLINE, 0, (SA*) pcliaddr, &len);
		printf("child %d, datagram from %s", getpid(), Sock_ntop(pcliaddr, len));
		printf(", to %s\n", Sock_ntop(myaddr, len));

		sendto(recvd_socket_info.sockfd, mesg, n, 0, pcliaddr, len);
	}
}

