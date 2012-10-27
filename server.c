#include "unpifiplus.h"
#include	"unprtt.h"
#include	<setjmp.h>
#include	<sys/time.h>

/*
*  1. Implement timeouts for the send and receive of filename and ephemeral port number
*
*  2. Error checking (SIGCHLD)
*/

struct socket_info{
	int sockfd;
	uint32_t ipaddress;
	uint32_t networkmask;
	uint32_t subnetaddress;
 	};
 	
struct socket_info intf_info[32];

/* Structures and variables for timeout mechanism */

static struct rtt_info   rttinfo;
static int	rttinit = 0;
static struct msghdr	msgsend, msgrecv;	/* assumed init to 0 */
static struct hdr {
  uint32_t	seq;	/* sequence # */
  uint32_t	ts;		/* timestamp when sent */
} sendhdr, recvhdr;

static void	sig_alrm(int signo);
static sigjmp_buf	jmpbuf;

ssize_t
dg_send_recv(int fd, const void *outbuff, size_t outbytes,
			 void *inbuff, size_t inbytes,
			 const SA *destaddr, socklen_t destlen)
{
	ssize_t			n;
	struct iovec	iovsend[2], iovrecv[2];
	struct itimerval timer, oldtimer;
	struct timeval tval;
	int x;

	if (rttinit == 0) {
		rtt_init(&rttinfo);		/* first time we're called */
		rttinit = 1;
		rtt_d_flag = 1;
	}
	//printf("\nFinished setting the rtt init");
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
	//printf("\ncalling signal");
	Signal(SIGALRM, sig_alrm);
	//printf("\ncalling rtt_newpack");
	rtt_newpack(&rttinfo);		/* initialize for this packet */
	//printf("\nIn dg_sed_recv function");
	//printf("The contents are : %s",msgsend.msg_iov[1].iov_base);
sendagain:
	sendhdr.ts = rtt_ts(&rttinfo);
	if(sendmsg(fd, &msgsend, 0))
	{
		//printf("\nCalling sendmsg in dg_send_recv");
	}
	//printf("The contents are : %s",msgsend.msg_iov[1].iov_base);
	//alarm(rtt_start(&rttinfo));	/* calc timeout value & start timer */
	tval.tv_sec = 0;
	//tval.tv_usec = 0;
	tval.tv_usec = rtt_start(&rttinfo); /* Converting milliseconds return value of rtt_start to microseconds */
	timer.it_interval = tval;
	timer.it_value = tval;
	if(setitimer(ITIMER_REAL, &timer, &oldtimer) == 0)
	{
		//printf("\nTimer started...");
	} 

	if (sigsetjmp(jmpbuf, 1) != 0) {
		if (rtt_timeout(&rttinfo) < 0) {
			err_msg("dg_send_recv: no response from Client, giving up");
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
		printf("\nThe server received : %s with sequence number:%d\n", msgrecv.msg_iov[1].iov_base,recvhdr.seq); 
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
	if(setitimer(ITIMER_REAL, &timer, &oldtimer) == 0)
	{
		//printf("\nTimer stopped...");
	} 
		/* 4calculate & store new RTT estimator values */
	rtt_stop(&rttinfo, rtt_ts(&rttinfo) - recvhdr.ts);

	return(n - sizeof(struct hdr));	/* return size of received datagram */
}

static void
sig_alrm(int signo)
{
	siglongjmp(jmpbuf, 1);
}

void ftp_server(struct socket_info this_socket, struct sockaddr *pcliaddr, socklen_t clilen, int noofintf, char filename)
{
	pid_t mypid;
	mypid = getpid();
	printf("\nServer Child %d: Reached forked ftp_server function\n", mypid);
	
	int n, i = 0, setflag = 0;
	int connfd;
	int counter=1,lc=0;
	FILE *fd;
	char mesg[MAXLINE], filecontent[MAXLINE], sendbuf[MAXLINE], recvbuf[MAXLINE+1];
	char str[INET_ADDRSTRLEN];
	const int on = 1;
	struct sockaddr_in IPserver, IPclient;
	struct sockaddr_in temp;
	socklen_t len;
	int sa_len = sizeof(IPserver);
	struct iovec	iovrecv[2], iovsend[2];
	
	//Variables for getpeername stuff
	socklen_t slen;
	struct sockaddr_storage temp_storage;
	char ipstr[INET6_ADDRSTRLEN];
	int port;	
	slen = sizeof(temp_storage);
	
	struct node *datalist;
	
	//Code to close all the sockets except Listening socket
	printf("\nServer Child %d: Closing all sockets except Listening socket: %d\n", mypid, this_socket.sockfd);
	for(i=0 ; i<=noofintf ; i++)
	{
		if(this_socket.sockfd == intf_info[i].sockfd)
			continue;
		close(intf_info[i].sockfd);
	}
	
	IPserver.sin_addr.s_addr = this_socket.ipaddress;
	IPserver.sin_port = htons(0);
	
	len = clilen;
	
	//Initializing IPclient
	inet_pton(AF_INET, Sock_ntop_host(pcliaddr, sizeof(*pcliaddr)), &(IPclient.sin_addr));
	IPclient.sin_port = ((struct sockaddr_in*)pcliaddr)->sin_port;
		
	//Code to check if the client is local or not
	if((IPclient.sin_addr.s_addr&this_socket.networkmask) == this_socket.subnetaddress)
	{
		printf("\nServer Child %d: The client and the server are local. Hence setting SO_DONTROUTE\n", mypid);
		setflag = 1;
	}
	else
	{
		printf("\nServer Child %d: The client and the server are not local\n", mypid);
	}
	
	//Creating and binding a new Socket for FTP application
	connfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(setflag == 1)
		setsockopt(connfd, SOL_SOCKET, SO_REUSEADDR|SO_DONTROUTE, &on, sizeof(on));
	else
		setsockopt(connfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	if( (bind(connfd, (SA *) &IPserver, sizeof(IPserver))) == -1)
	{
		printf("\nServer Child %d: ERROR :Unable to bind socket : %d", mypid, connfd);
		printf("\nServer Child %d: ERROR :The erorr number is as follows : %d", mypid, errno);
		exit(0);
	}
	
	if (getsockname(connfd, &temp, &sa_len) == -1)
	{
		printf("\nServer Child %d: ERROR : Unable to getsockname", mypid);
		printf("\nServer Child %d: ERROR :The erorr number is as follows : %d", mypid, errno);
		exit(0);
	}
	
	//Printing out the details of the server
	inet_ntop(AF_INET, &(temp.sin_addr.s_addr), str, INET_ADDRSTRLEN);
	printf("\nServer Child %d: IPserver is %s", mypid, str);
	printf("\nServer Child %d: Port Number of IPserver is %d\n", mypid, temp.sin_port);
	
	IPserver.sin_port = temp.sin_port;
	
	IPclient.sin_family = AF_INET;
	setsockopt(connfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	if(connect(connfd, (struct sockaddr *) &IPclient, sizeof(IPclient)) == -1)
	{
		printf("\nERROR :Was unable to connect on socket %d. Please check the client", connfd);
		printf("\nERROR :The error number is as follows : %d. Exitting\n", errno);
		exit(0);
	}
	else
		printf("\nServer Child %d: Connected to client on socket : %d\n", mypid, connfd);
	
	//Printing out the details of the connected client
	//GetPeerName stuff
	bzero(&temp_storage, sizeof(temp_storage));
	getpeername(connfd, (struct sockaddr*)&temp_storage, &slen);
	struct sockaddr_in *sock_temp = (struct sockaddr_in *)&temp_storage;
	port = ntohs(sock_temp->sin_port);
	inet_ntop(AF_INET, &sock_temp->sin_addr, ipstr, sizeof ipstr);
	
	inet_ntop(AF_INET, &(IPclient.sin_addr.s_addr), str, INET_ADDRSTRLEN);
	printf("\nServer Child %d: IP Address of the connected client is %s", mypid, ipstr);
	printf("\nServer Child %d: Port Number of the connected client is %d\n", mypid, port);
	
	//Sending the new ephemeral port number to client
	sprintf(mesg, "%d", IPserver.sin_port);
	sendto(this_socket.sockfd, mesg, sizeof(mesg), 0, pcliaddr, len);
	close(this_socket.sockfd);
	
	
	//***Start of Part 2 code**************************************************************************************	
	//*************************************************************************************************************
	
	msgrecv.msg_name = NULL;
	msgrecv.msg_namelen = 0;
	msgrecv.msg_iov = iovrecv;
	msgrecv.msg_iovlen = 2;
	iovrecv[0].iov_base = &recvhdr;
	iovrecv[0].iov_len = sizeof(struct hdr);
	iovrecv[1].iov_base = recvbuf;
	iovrecv[1].iov_len = sizeof(recvbuf);
	
	sendbuf[0] = 'A';
	sendbuf[1] = 'C';
	sendbuf[2] = 'K';
	msgsend.msg_name = NULL;
	msgsend.msg_namelen = 0;
	msgsend.msg_iov = iovsend;
	msgsend.msg_iovlen = 2;
	iovsend[1].iov_base = sendbuf;
	iovsend[1].iov_len = sizeof(sendbuf);
	if((n = recvmsg(connfd, &msgrecv, 0))>0)
	{
		printf("\nThe Filename received by server is: %s with sequence number:%d\n", msgrecv.msg_iov[1].iov_base,recvhdr.seq); 
		iovsend[0].iov_base = msgrecv.msg_iov[0].iov_base;
		iovsend[0].iov_len = sizeof(struct hdr);
		if(sendmsg(connfd, &msgsend, 0))
		{
			printf("\nACK sent for filename");
		}
	}
	else
	{
		printf("The recvmsg failed while receiving filename from client");
	}
	fd = fopen(msgrecv.msg_iov[1].iov_base, "r");
	
	if(fd == NULL)
	{
		printf("File open error for: %s file", mesg);
	}
	else
	{
		while(fgets(filecontent, 20, fd)!= NULL)
		{
			//printf("\ncalling dg send recv");
			if(lc == 1)
			{
				datalist = makelist(1);
			}
			while(lc > 0)
			{
				
			}
			dg_send_recv(connfd, filecontent, sizeof(filecontent), recvbuf, MAXLINE, (const SA *)ipstr, sizeof(ipstr));  
			memset(&filecontent[0],0,sizeof(filecontent));
		}
		dg_send_recv(connfd, "\0", 1, recvbuf, MAXLINE, (const SA *)ipstr, sizeof(ipstr));
		fclose(fd);			
	}	
	
	//***End of Part 2 code****************************************************************************************
	//*************************************************************************************************************
	
	printf("\nReached the end of the server child code\n");
	return;
}

int main(int argc, char **argv)
{
	printf("\nServer Program \n");
	int maxfdp1 = 0; 
	fd_set rset;
	int k;
	
	int sockfd = 0;
	int i = 0, result = 0; 
	pid_t	child_pid;
	const int on = 1;	
	struct ifi_info	*ifi, *ifihead;
	struct sockaddr_in *sa;
	struct sockaddr_in cliaddr;
	char mesg[MAXLINE];
	
	int server_port;
	int window_size;
	
	int len = sizeof(cliaddr);
	bzero(&cliaddr, sizeof(cliaddr));
	
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
	int noofintf = 0;
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
			printf("\n server : ERROR :Unable to bind socket : %d", sockfd);
			printf("\n server : ERROR :The error number is as follows : %d. Exitting", errno);
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
			perror("\nServer Parent: ERROR :Something wrong with select. Resetting\n");
			exit(0);
		}
		
		for(i=0; i<=noofintf ; i++)
		{
			if(FD_ISSET(intf_info[i].sockfd, &rset))
			{
				bzero(&mesg, sizeof(mesg));
				printf("\nServer Parent: Incoming datagram on socket : %d\n", intf_info[i].sockfd);
				k = recvfrom(intf_info[i].sockfd, mesg, MAXLINE, 0, (struct sockaddr *) &cliaddr, &len);
				
				
				//TODO Implement Timeouts here; Timeout value should be small
				printf("\nServer Parent: The client has requested the file %s\n", mesg);
				sendto(intf_info[i].sockfd, mesg, k, 0, (struct sockaddr *) &cliaddr, len);
				printf("\nServer Parent: Sent an acknowledgement to the client. Waiting for response.\n");
				recvfrom(intf_info[i].sockfd, mesg, MAXLINE, 0, (struct sockaddr *) &cliaddr, &len);
				printf("\nServer Parent: Received a response. Forking a child to handle this.\n");
				
				child_pid = fork();
			
				if(child_pid >= 0) // fork was successful
				{
					if(child_pid == 0) // child process
					{
					    ftp_server(intf_info[i], (struct sockaddr *) &cliaddr, sizeof(cliaddr), noofintf, mesg);
					    exit(0);
					}
					else //Parent process 
					{
					    //printf("\nServer Parent: Parent Process");
					}
				}
				else // fork failed
				{
					printf("\nServer Parent: ERROR :Forking the child failed. Exitting\n");
					exit(0);
				}
			}
		}
	}
}


