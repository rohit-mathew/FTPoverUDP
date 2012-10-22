#include "unpifiplus.h"

struct socket_info{
	uint32_t ipaddress;
	uint32_t networkmask;
	uint32_t subnetaddress;
 	}; 	
 	


void ftp_cli(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servlen)
{
	int	n;
	char	sendline[MAXLINE], recvline[MAXLINE + 1];
	
	

	while (fgets(sendline, MAXLINE, fp) != NULL) {

		sendto(sockfd, sendline, strlen(sendline), 0, NULL, 0);

		n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL);

		recvline[n] = 0;	/* null terminate */
		fputs(recvline, stdout);
	}
}
	
int main(int argc, char **argv)
{
	printf("\nWelcome to the Client Program \n");
	int sockfd = 0;
	struct sockaddr_in servaddr;
	const int on = 1;	
	struct ifi_info	*ifi, *ifihead;
	struct sockaddr_in *sa;
	struct sockaddr_in IPclient, IPserver;
	struct sockaddr_in temp;
	struct socket_info intf_info[32];
	int i = 0, setflag = 0;
	int sa_len = sizeof(IPclient);
	
	//Variables for getpeername stuff
	socklen_t slen;
	struct sockaddr_storage temp_storage;
	char ipstr[INET6_ADDRSTRLEN];
	int port;	
	slen = sizeof(temp_storage);
	
	//Client parameters
	char serverIP[INET_ADDRSTRLEN];
	int server_port;
	char filename[128];
	int window_size;
	int randseed; //TODO
	int lossprob;
	int meanu;
	
	//Zeroing out IPserver and Ipclient
	bzero(&IPclient, sizeof(IPclient));
	bzero(&IPserver, sizeof(IPserver));
	
	bzero(&temp, sizeof(temp));
	
	//Read client.in file
	//TODO Error check here for wrong values input from user
	FILE *fr = fopen ("client.in", "r");  
	
	if(fr == NULL)
	{
		printf("ERROR : Unable to open file 'client.in',  Please check. \n");
		exit(0);
	}
	
	fscanf (fr, "%s", &serverIP);
	fscanf (fr, "%d", &server_port); 
	fscanf (fr, "%s", &filename);
	fscanf (fr, "%d", &window_size);
	fscanf (fr, "%d", &randseed);
	fscanf (fr, "%d", &lossprob);
	fscanf (fr, "%d", &meanu);

	fclose(fr);
	
	printf("\nDetails from client.in file :-\n");
	printf("\nIP Address : \t\t\t%s", serverIP);
	printf("\nFile Name : \t\t\t%d", server_port);
	printf("\nIP Address : \t\t\t%s", filename);
	printf("\nWindow Size : \t\t\t%d", window_size);
	printf("\nRandom Seed : \t\t\t%d", randseed);
	printf("\nProbability of loss : \t\t%d", lossprob);
	printf("\nMean (in milliseconds) : \t%d", meanu);
	
	//Iterating through all the interfaces
	int noofintf = 0;
		
	for (ifihead = ifi = Get_ifi_info_plus(AF_INET, 1);
		 ifi != NULL; ifi = ifi->ifi_next) 
	{
		sa = (struct sockaddr_in *) ifi->ifi_addr;
		intf_info[noofintf].ipaddress = sa->sin_addr.s_addr;
		
		sa = (struct sockaddr_in *) ifi->ifi_ntmaddr;
		intf_info[noofintf].networkmask = sa->sin_addr.s_addr;
		
		intf_info[noofintf].subnetaddress = intf_info[noofintf].networkmask&intf_info[noofintf].ipaddress; //TODO
		
		noofintf = noofintf+1;
	}
	
	char str[128];

	printf("\n\nThere are %d interfaces.\n", noofintf);	
	noofintf = noofintf-1;	
	
	for(i=0; i<=noofintf ; i++)
	{
		printf("\nThese are the details for interface no : %d\n", i);
		inet_ntop(AF_INET, &(intf_info[i].ipaddress), str, INET_ADDRSTRLEN);
		printf("IP Address is \t\t%s\n", str);
		inet_ntop(AF_INET, &(intf_info[i].networkmask), str, INET_ADDRSTRLEN);
		printf("Network Mask is \t%s\n", str);
		inet_ntop(AF_INET, &(intf_info[i].subnetaddress), str, INET_ADDRSTRLEN);
		printf("Subnet Address is \t%s\n", str);
	}
	
	//Setting the values of IPserver TODO
	inet_pton(AF_INET, serverIP, &(IPserver.sin_addr));
	
	printf("\nSetting the IP address and the port number of the server (IPserver)\n");
	//Check if the server is on loopback address
	if(inet_addr(serverIP) == inet_addr("127.0.0.1"))	
	{
		printf("\nThe server is local (on the same host as the client)\n");
		inet_pton(AF_INET, serverIP, &(IPclient.sin_addr));
		setflag = 1;		
	}	
	//TODO Check if the server is the same subnet
	for(i=0; i<=noofintf ; i++)
	{
		if((IPserver.sin_addr.s_addr&intf_info[i].networkmask) == intf_info[i].subnetaddress)
		{
			inet_ntop(AF_INET, &(intf_info[i].ipaddress), str, INET_ADDRSTRLEN);
			printf("\nThe server is on the same subnet as the client IP address %s\n", str);
			IPclient.sin_addr.s_addr = intf_info[i].ipaddress;
			setflag = 1;
		}
	}
	
	//If the server is not the same host nor if it is in the same subnet
	if(setflag == 0)
	{
		printf("\nThe server is not on the same host nor is it on the same subnet\n");
		IPclient.sin_addr.s_addr = intf_info[0].ipaddress;
	}	
	
	IPclient.sin_port = htons(0);
	IPclient.sin_family = AF_INET;
	
	//Creating a new socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	
	if( (bind(sockfd, (struct sockaddr *) &IPclient, sa_len)) == -1)
	{
		printf("\nERROR :Unable to bind socket : %d", sockfd);
		printf("\nERROR :The error number is as follows : %d. \nExitting", errno);
		exit(0);
	}
	
	if (getsockname(sockfd, &temp, &sa_len) == -1)
	{
		printf("\nERROR : Unable to getsockname");
		printf("\nERROR :The erorr number is as follows : %d", errno);
		exit(0);
	}
	
	//Printing the IPclient details
	inet_ntop(AF_INET, &(temp.sin_addr.s_addr), str, INET_ADDRSTRLEN);
	printf("\nIP Address of the client (IPclient) is \t\t%s\n", str);
	printf("Port Number (Well known) of the client (IPclient) is \t%d\n", temp.sin_port);	
	
	IPserver.sin_port = htons(server_port);
	IPserver.sin_family = AF_INET;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	//Connecting to the server	
	if(connect(sockfd, (struct sockaddr *) &IPserver, sizeof(IPserver)) == -1)
	{
		printf("\nERROR :Was unable to connect. Please check the server");
		printf("\nERROR :The error number is as follows : %d. Exitting\n", errno);
		//exit(0);
	}
	
	//GetPeerName stuff
	bzero(&temp_storage, sizeof(temp_storage));
	getpeername(sockfd, (struct sockaddr*)&temp_storage, &slen);
	struct sockaddr_in *sock_temp = (struct sockaddr_in *)&temp_storage;
	port = ntohs(sock_temp->sin_port);
	inet_ntop(AF_INET, &sock_temp->sin_addr, ipstr, sizeof ipstr);
	
	printf("\nIP Address of the server (IPserver) is \t\t%s\n", ipstr);
	printf("Port Number of the server (IPserver) is \t%d\n", port);
	
	sendto(sockfd, filename, strlen(filename), 0, NULL, 0);
	//n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL);
	
	
	//ftp_cli(stdin, sockfd, (struct sockaddr *) &IPserver, sizeof(IPserver));
	printf("\nReached the end of client program, exitting\n");
}
