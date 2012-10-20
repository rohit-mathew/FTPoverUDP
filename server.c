/* include udpserv1 */
#include "unpifiplus.h"



struct socket_info{
	int sockfd;
	uint32_t ipaddress;
	uint32_t networkmask;
	uint32_t subnetaddress;
 	};
 	
struct socket_info intf_info[32];

void mydg_echo(struct socket_info this_socket, struct sockaddr *pcliaddr, socklen_t clilen, int noofintf)
{
	printf("\nChild %d: Reached mydg_echo\n", getpid());
	int n, i = 0;
	char mesg[MAXLINE];
	char str[INET_ADDRSTRLEN];
	struct sockaddr_in myaddr;
	socklen_t	len;
	
	//Code to close all the sockets except this_socket.sockfd
	printf("\nChild %d: Closing all sockets except current socket: %d\n", getpid(), this_socket.sockfd);
	for(i=0 ; i<=noofintf ; i++)
	{
		if(this_socket.sockfd == intf_info[i].sockfd)
			continue;
		printf("\nChild %d: Closing socket: %d\n", getpid(), intf_info[i].sockfd);
		close(intf_info[i].sockfd);
	}
	
	myaddr.sin_addr.s_addr = this_socket.ipaddress;
	
	//inet_ntop(AF_INET, &(myaddr.sin_addr.s_addr), str, INET_ADDRSTRLEN);
	//printf("\nIP Address is \t%s\n", str);
	
	for ( ; ; ) 
	{
		len = clilen;
		n = recvfrom(this_socket.sockfd, mesg, MAXLINE, 0, pcliaddr, &len);
		//printf("child %d, datagram from %s", getpid(), Sock_ntop(pcliaddr, len));
		//printf(", to %s\n", Sock_ntop(myaddr, len));
		sendto(this_socket.sockfd, mesg, n, 0, pcliaddr, len);
	}
}

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
	struct sockaddr_in cliaddr;
	
	int server_port;
	int window_size;
	
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
		intf_info[noofintf].networkmask = sa->sin_addr.s_addr;
		
		intf_info[noofintf].subnetaddress = intf_info[noofintf].networkmask&intf_info[noofintf].ipaddress;
		
		noofintf = noofintf+1;
	}
	
	//Printing all the values out
	
	char str[128];

	printf("\nThere are %d interfaces.\n", noofintf);	
	noofintf = noofintf-1;	
	
	for(i=0; i<=noofintf ; i++)
	{
		printf("These are the details for interface no : %d\n", i);
		inet_ntop(AF_INET, &(intf_info[i].ipaddress), str, INET_ADDRSTRLEN);
		printf("IP Address is \t%s\n", str);
		inet_ntop(AF_INET, &(intf_info[i].networkmask), str, INET_ADDRSTRLEN);
		printf("Network Mask is %s\n", str);
		inet_ntop(AF_INET, &(intf_info[i].subnetaddress), str, INET_ADDRSTRLEN);
		printf("Subnet Address is is %s\n", str);
		
		//Finding the maximum socket no
		if(intf_info[i].sockfd > maxfdp1)
			maxfdp1 = intf_info[i].sockfd;
	}
	
	maxfdp1 = maxfdp1 + 1;
	
	while(1)
	{	
		FD_ZERO(&rset);
		for(i=0; i<=noofintf ; i++)
		{
			FD_SET(intf_info[i].sockfd, &rset);
		}
		printf("\nServer Parent: Waiting at select \n");
		result = select(maxfdp1, &rset, NULL, NULL, NULL);
		
		if(result == -1)
		{
			printf("\nServer Parent: Something wrong with select. Resetting \n");
			//continue;
		}
		
		for(i=0; i<=noofintf ; i++)
		{
			if(FD_ISSET(intf_info[i].sockfd, &rset))
			{
				printf("\nServer Parent: Incoming datagram on socket : %d. Forking a child to handle this.\n", intf_info[i].sockfd);
				child_pid = fork();
			
				if(child_pid >= 0) // fork was successful
				{
					if(child_pid == 0) // child process
					{
					    mydg_echo(intf_info[i], (struct sockaddr *) &cliaddr, sizeof(cliaddr), noofintf);
					}
					else //Parent process TODO Anything to be done here
					{
					    printf("\nServer Parent: Parent Process");
					}
				}
				else // fork failed
				{
					printf("\nServer Parent: Forking the child failed. Exitting\n");
					exit(0);
				}
			}
		}
	}
}


