#include "unpifiplus.h"
#include	"unprtt.h"
#include	<setjmp.h>
#include	<sys/time.h>

float random_gen()
{
        struct timeval  tv;

        if (gettimeofday(&tv, NULL) == -1) {
                perror("gettimeofday error: ");
        }
        srand48(tv.tv_usec); /* Setting the seed in microseconds since we are dealing in ms and microseconds */ 
        return drand48();
}
/*
TODO

1. Check the error conditions on all the functions
2. 
*/
struct socket_info{
	uint32_t ipaddress;
	uint32_t networkmask;
	uint32_t subnetaddress;
 	}; 	
 	static struct msghdr	msgsend, msgrecv;	/* assumed init to 0 */
static struct hdr {
	uint32_t	seq;	/* sequence # */
	uint32_t	ts;		/* timestamp when sent */
	} sendhdr, recvhdr;

static struct rtt_info   rttinfo;
static int	rttinit = 0;
static void	sig_alrm(int signo);
static sigjmp_buf jmpbuf;

ssize_t
dg_send_recv(int fd, const void *outbuff, size_t outbytes,
			 void *inbuff, size_t inbytes,
			 const SA *destaddr, socklen_t destlen)
{
	ssize_t			n;
	struct iovec	iovsend[2], iovrecv[2];
	struct itimerval timer, oldtimer;
	struct timeval tval;

	if (rttinit == 0) {
		rtt_init(&rttinfo);		/* first time we're called */
		rttinit = 1;
		rtt_d_flag = 1;
	}

	sendhdr.seq++;
	msgsend.msg_name = NULL;
	msgsend.msg_namelen = 0;
	msgsend.msg_iov = iovsend;
	msgsend.msg_iovlen = 2;
	iovsend[0].iov_base = &sendhdr;
	iovsend[0].iov_len = sizeof(struct hdr);
	iovsend[1].iov_base = outbuff;
	iovsend[1].iov_len = outbytes;

	msgrecv.msg_name = NULL;
	msgrecv.msg_namelen = 0;
	msgrecv.msg_iov = iovrecv;
	msgrecv.msg_iovlen = 2;
	iovrecv[0].iov_base = &recvhdr;
	iovrecv[0].iov_len = sizeof(struct hdr);
	iovrecv[1].iov_base = inbuff;
	iovrecv[1].iov_len = inbytes;
/* end dgsendrecv1 */

/* include dgsendrecv2 */
	Signal(SIGALRM, sig_alrm);
	rtt_newpack(&rttinfo);		/* initialize for this packet */
	//printf("In dg_sed_recv function:");
	//printf("The contents are : %s",msgsend.msg_iov[1].iov_base);
sendagain:
	sendhdr.ts = rtt_ts(&rttinfo);
	sendmsg(fd, &msgsend, 0);

	//alarm(rtt_start(&rttinfo));	/* calc timeout value & start timer */
	tval.tv_sec = 0;
	tval.tv_usec = rtt_start(&rttinfo)*1000; /* Converting milliseconds return value of rtt_start to microseconds */
	timer.it_interval = tval;
	timer.it_value = tval;
	setitimer(ITIMER_REAL, &timer, &oldtimer); 
	if (sigsetjmp(jmpbuf, 1) != 0) {
		if (rtt_timeout(&rttinfo) < 0) {
			err_msg("dg_send_recv: no response from server, giving up");
			rttinit = 0;	/* reinit in case we're called again */
			errno = ETIMEDOUT;
			return(-1);
		}
		goto sendagain;
	}

	/*do {
		n = recvmsg(fd, &msgrecv, 0);
	} while (n < sizeof(struct hdr) || recvhdr.seq != sendhdr.seq);
	*/
	if((n = recvmsg(fd, &msgrecv, 0))>0)
	{
		printf("\nThe Client received : %s with sequence number:%d\n", msgrecv.msg_iov[1].iov_base,recvhdr.seq); 
	}
	else
	{
		printf("The recvmsg is failed in dg_send_recv");
	}
	msgrecv.msg_iov = NULL;
	//alarm(0);			/* stop SIGALRM timer */
	tval.tv_sec = 0;
	tval.tv_usec = 0;
	timer.it_interval = tval;
	timer.it_value = tval;
	setitimer(ITIMER_REAL, &timer, &oldtimer); 
		/* 4calculate & store new RTT estimator values */
	rtt_stop(&rttinfo, rtt_ts(&rttinfo) - recvhdr.ts);

	return(n - sizeof(struct hdr));	/* return size of received datagram */
}

static void
sig_alrm(int signo)
{
	siglongjmp(jmpbuf, 1);
}


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
	int i = 0, setflag = 0, n = 0;
	int sa_len = sizeof(IPclient);
	char	sendline[MAXLINE], recvline[MAXLINE + 1], recvcontent[MAXLINE], sendcontent[MAXLINE];
	int testip = inet_addr("0.0.0.0");
	
	
	
	//Variables for getpeername stuff
	socklen_t slen;
	struct sockaddr_storage temp_storage;
	char ipstr[INET6_ADDRSTRLEN];
	int port,x;		
	slen = sizeof(temp_storage);
	
	//Client parameters
	char serverIP[INET_ADDRSTRLEN];
	int server_port;
	char filename[128];
	int window_size;
	int randseed; //TODO
	int lossprob;
	int meanu;
	
	float result;
	struct iovec	iovsend[2], iovrecv[2];
	
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
	//Check if the server is on the same host
	if(setflag == 0)
	{
		for(i=0; i<=noofintf ; i++)
		{
			if(inet_addr(serverIP) == intf_info[i].ipaddress)
			{
				memset(recvline,0,sizeof(recvline));
				printf("\nThe server and the client are both on the same host\n");
				inet_pton(AF_INET, "127.0.0.1", &(IPclient.sin_addr));
				inet_pton(AF_INET, "127.0.0.1", &(IPserver.sin_addr));
				setflag = 1;
				break;
			}
		}
	}
	
	//TODO Check if the server is the same subnet
	if(setflag == 0)
	{
		for(i=0; i<=noofintf ; i++)
		{
			if((IPserver.sin_addr.s_addr&intf_info[i].networkmask) == intf_info[i].subnetaddress)
			{
				inet_ntop(AF_INET, &(intf_info[i].ipaddress), str, INET_ADDRSTRLEN);
				printf("\nThe server is on the same subnet as the client IP address %s", str);
				//Doing longest prefix matching
				if(intf_info[i].ipaddress >= testip)
				{
					IPclient.sin_addr.s_addr = intf_info[i].ipaddress;
					testip = intf_info[i].ipaddress;
				}
				setflag = 1;
			}
		}
		if(setflag == 1)
		{
			inet_ntop(AF_INET, &(testip), str, INET_ADDRSTRLEN);
			printf("\nThe ip address %s was chosen as it had the longest matching prefix.\n", str);
		}
	}
	
	//If the server is not the same host nor if it is in the same subnet
	if(setflag == 0)
	{
		printf("\nThe server is not on the same host nor is it on the same subnet\n");
		IPclient.sin_addr.s_addr = intf_info[1].ipaddress;
	}	
	
	IPclient.sin_port = htons(0);
	IPclient.sin_family = AF_INET;
	
	//Creating a new socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(setflag == 1)
	{
		setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR|SO_DONTROUTE, &on, sizeof(on));
		printf("\nThe client and the server are local. Hence setting SO_DONTROUTE\n");
	}
	else
		setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	
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
	printf("Port Number of the client (IPclient) is \t%d\n", temp.sin_port);	
	
	IPserver.sin_port = htons(server_port);
	IPserver.sin_family = AF_INET;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	//Connecting to the server	
	if(connect(sockfd, (struct sockaddr *) &IPserver, sizeof(IPserver)) == -1)
	{
		perror("\nERROR :Exitting, Was unable to connect. Please check the server");
		exit(0);
	}
	else
		printf("\nConnection request send to server on well known port.\n");
	
	//GetPeerName stuff
	bzero(&temp_storage, sizeof(temp_storage));
	getpeername(sockfd, (struct sockaddr*)&temp_storage, &slen);
	struct sockaddr_in *sock_temp = (struct sockaddr_in *)&temp_storage;
	port = ntohs(sock_temp->sin_port);
	inet_ntop(AF_INET, &sock_temp->sin_addr, ipstr, sizeof ipstr);
	
	printf("\nIP Address of the server (IPserver) is \t\t\t\t\t%s\n", ipstr);
	printf("Port Number (well known port number) of the server (IPserver) is \t%d\n", port);
		
	sendto(sockfd, filename, strlen(filename), 0, NULL, 0);
	if( (n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL)) > 0)
		recvline[n] = 0;
	else
	{
		perror("\nLooks like the server is down, exitting. Error Message");
		exit(0);
	}
	sendto(sockfd, recvline, strlen(recvline), 0, NULL, 0);
	
	//Server forks during this period
	bzero(&recvline, sizeof(recvline));
	recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL);
	
	
	printf("\nReceived a new port number from the server : %s.\n", recvline);
	//Breaking the previous connection
	IPserver.sin_family = AF_UNSPEC;
	if(connect(sockfd, (struct sockaddr *) &IPserver, sizeof(IPserver)) == -1)
	{
		printf("\nERROR :Was unable to connect to the server on new port. Please check the server");
		printf("\nERROR :The error number is as follows : %d. Exitting\n", errno);
		exit(0);
	}
	else
		printf("\nBroke the connection to the server\n");
	
	//Making a new connection
	IPserver.sin_port = strtol(recvline, NULL,10);
	IPserver.sin_family = AF_INET;
	if(connect(sockfd, (struct sockaddr *) &IPserver, sizeof(IPserver)) == -1)
	{
		printf("\nERROR :Was unable to connect to the server on new port. Please check the server");
		printf("\nERROR :The error number is as follows : %d. Exitting\n", errno);
		exit(0);
	}
	else
		printf("\nConnected to the server with the new port numer.\n");
	
	
	//GetPeerName stuff
	bzero(&temp_storage, sizeof(temp_storage));
	getpeername(sockfd, (struct sockaddr*)&temp_storage, &slen);
	struct sockaddr_in *sock1_temp = (struct sockaddr_in *)&temp_storage;
	port = ntohs(sock_temp->sin_port);
	inet_ntop(AF_INET, &sock_temp->sin_addr, ipstr, sizeof ipstr);
	
	printf("\nIP Address of the server (IPserver) is \t\t%s\n", ipstr);
	printf("Port Number of the server (IPserver) is \t%d\n", port);
	
	
	
	//********************************************************************************************************
	//@Chaitanya This is some sample code, that just echoes the file name
	//@Chaitanya sockfd is the final connected socket
	                  
	//sendto(sockfd, filename, strlen(filename), 0, NULL, 0);
	//n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL);
	
	//printf("\nReceived from server : %s", recvline);
	
	dg_send_recv(sockfd, filename, strlen(filename), recvcontent, sizeof(recvcontent), (const SA *)ipstr, sizeof(ipstr));
	
	while(1)
	{
			memset(&recvcontent[0],0,sizeof(recvcontent));
			msgrecv.msg_name = NULL;
			msgrecv.msg_namelen = 0;
			msgrecv.msg_iov = iovrecv;
			msgrecv.msg_iovlen = 2;
			iovrecv[0].iov_base = &recvhdr;
			iovrecv[0].iov_len = sizeof(struct hdr);
			iovrecv[1].iov_base = recvcontent;
			iovrecv[1].iov_len = sizeof(recvcontent);
			
			sendcontent[0] = 'A';
			sendcontent[1] = 'C';
			sendcontent[2] = 'K';
			msgsend.msg_name = NULL;
			msgsend.msg_namelen = 0;
			msgsend.msg_iov = iovsend;
			msgsend.msg_iovlen = 2;
			
			n = recvmsg(sockfd, &msgrecv, 0);
			printf("\n Received from server : %s with sequence number: %d\n",msgrecv.msg_iov[1].iov_base,recvhdr.seq);
			iovsend[0].iov_base = msgrecv.msg_iov[0].iov_base;
			iovsend[0].iov_len = sizeof(struct hdr);
			iovsend[1].iov_base = &sendcontent;
			iovsend[1].iov_len = sizeof(sendcontent);
			result = random_gen();
			//printf("The random number is:%f",result);
			//result =0.1;
			if(result > 0.3)
			{
				if((x=strcmp(msgrecv.msg_iov[1].iov_base, "\0"))==0)
				{
					sendmsg(sockfd, &msgsend ,0);
					break;
				}
				if(sendmsg(sockfd, &msgsend ,0))
				{
					//printf("Sending reply to server");
				}
				//printf("\n Sent ACK to the server for datagram with sequence number: %d\n",recvhdr.seq);
			}
			//result = result + 1;
	}
	
	printf("\nReached the end of client program, exitting\n");
}
