/*
 * Lab problem set for INP course
 * by Chun-Ying Huang <chuang@cs.nctu.edu.tw>
 * License: GPLv2
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
// for opening directory
#include <dirent.h>
#include <iconv.h>

#define err_quit(m) { perror(m); exit(-1); }

#define NIPQUAD(s)	((unsigned char *) &s)[0], \
					((unsigned char *) &s)[1], \
					((unsigned char *) &s)[2], \
					((unsigned char *) &s)[3]
#define SIZE 1024
#define RE_TIME 1 // retransmission time

static int s = -1;
static struct sockaddr_in sin;
static unsigned seq;
static unsigned count = 0;
static unsigned re_count = 0;
int file_size;

//pending buffers
char buffer[SIZE];

typedef struct {
	unsigned seq;
	struct timeval tv;
}	ping_t;

double tv2ms(struct timeval *tv) {
	return 1000.0*tv->tv_sec + 0.001*tv->tv_usec;
}

typedef struct {
	unsigned file_num;
	unsigned seq;
	char buf[SIZE];
	int total_size;
}	packet_t;



void do_send(int sig) {
	if(sig == SIGALRM) {
		unsigned char pack_buf[2048];
		packet_t *p = (packet_t*) pack_buf;
		p->file_num = count;
		p->seq = seq;
		p->total_size=file_size;
		int n;
		// resend
		printf("[RESENDING] Data: %d\n", p->seq);
		if(seq==-1){
			n=sendto(s, p, sizeof(*p)+sizeof(int)+16, 0, (struct sockaddr*)&sin, sizeof(sin));
			n=sendto(s, p, sizeof(*p)+sizeof(int)+16, 0, (struct sockaddr*)&sin, sizeof(sin));
			n=sendto(s, p, sizeof(*p)+sizeof(int)+16, 0, (struct sockaddr*)&sin, sizeof(sin));
			n=sendto(s, p, sizeof(*p)+sizeof(int)+16, 0, (struct sockaddr*)&sin, sizeof(sin));
			n=sendto(s, p, sizeof(*p)+sizeof(int)+16, 0, (struct sockaddr*)&sin, sizeof(sin));
		}else{
			memcpy(p->buf, buffer, SIZE);
			n = sendto(s, p, sizeof(*p)+sizeof(int)+16, 0, (struct sockaddr*)&sin, sizeof(sin));
			n = sendto(s, p, sizeof(*p)+sizeof(int)+16, 0, (struct sockaddr*)&sin, sizeof(sin));
			n = sendto(s, p, sizeof(*p)+sizeof(int)+16, 0, (struct sockaddr*)&sin, sizeof(sin));
			n = sendto(s, p, sizeof(*p)+sizeof(int)+16, 0, (struct sockaddr*)&sin, sizeof(sin));
			n = sendto(s, p, sizeof(*p)+sizeof(int)+16, 0, (struct sockaddr*)&sin, sizeof(sin));
			
		}
		
		if (n == -1)
		{
			perror("[ERROR] sending data to the server.");
			exit(1);
		}
		alarm(RE_TIME);
	}
	re_count++;
	if(re_count > 20){
		exit(0);
		//alarm(0);
	}

}

void send_file_data(FILE* fp, int sockfd, struct sockaddr_in addr)
{
	int n;
	int rlen;
	char rbuf[SIZE];
	// packet type
	unsigned char pack_buf[SIZE*2];
	packet_t *p = (packet_t*) pack_buf;
	p->file_num = count;
	
	
	bzero(p->buf, SIZE);
	// receiveing
	struct sockaddr_in csin;
	socklen_t csinlen = sizeof(csin);
	// Sending the data

	fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
	printf("file size=%ld\n",file_size);
	rewind(fp);
    
	//sendto(s,&file_size, sizeof(long), 0, (struct sockaddr*) &sin, sizeof(sin));
	int ack;
	/*alarm(RE_TIME);
    seq = -1;
	while((rlen = recvfrom(s, &ack, sizeof(int), 0, (struct sockaddr*) &csin, &csinlen)) < 0||ack!=-1) {
		
	}
	// cancel the alarm
	alarm(0);*/
	int total=0;
	seq=0;
	p->seq = 0;
	while (fread(p->buf,1, SIZE, fp)>0)
	{	
		p->total_size=file_size;
		re_count=0;
		memcpy(buffer, p->buf, SIZE);
		//printf("[CLIENT-SENDING] Data: %s\n", p->buf);
		printf("[CLIENT-SENDING] Data:%d\n",seq);
		if(total+SIZE>=file_size){
			for(int i=0;i<10;i++){
				n = sendto(sockfd, p, sizeof(*p)+sizeof(int)+16, 0, (struct sockaddr*)&addr, sizeof(addr));
			}
			total+=sizeof(p->buf);
			continue;
		}
		n = sendto(sockfd, p, sizeof(*p)+sizeof(int)+16, 0, (struct sockaddr*)&addr, sizeof(addr));
		n = sendto(sockfd, p, sizeof(*p)+sizeof(int)+16, 0, (struct sockaddr*)&addr, sizeof(addr));
		n = sendto(sockfd, p, sizeof(*p)+sizeof(int)+16, 0, (struct sockaddr*)&addr, sizeof(addr));
		n = sendto(sockfd, p, sizeof(*p)+sizeof(int)+16, 0, (struct sockaddr*)&addr, sizeof(addr));
		n = sendto(sockfd, p, sizeof(*p)+sizeof(int)+16, 0, (struct sockaddr*)&addr, sizeof(addr));
		
		if (n == -1)
		{
			perror("[CLI-ERROR] sending data to the server.");
			exit(1);
		}
		// try to retransmit
		
		alarm(RE_TIME);
		
		while((rlen = recvfrom(s, &ack, sizeof(int), 0, (struct sockaddr*) &csin, &csinlen)) < 0||ack!=seq) {
				
		}
		alarm(0);
			
		
		total+=sizeof(p->buf);
		p->seq++;
		seq++;
		bzero(p->buf, SIZE);
		
	}
	printf("file size=%ld\n",file_size);
	printf("real size=%d\n",total);

	

	printf("sended END, current: %d files\n", count);

	fclose(fp);
	count++;
	//exit(0);
}

// example CLIENT
int main(int argc, char *argv[]) {
	if(argc != 5) {
		return -fprintf(stderr, "usage: %s <path-to-store-files> <total-number-of-files> <port> <ip>\n", argv[0]);
	}

	srand(time(0) ^ getpid());

	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(strtol(argv[argc-2], NULL, 0));
	if(inet_pton(AF_INET, argv[argc-1], &sin.sin_addr) != 1) {
		return -fprintf(stderr, "** cannot convert IPv4 address for %s\n", argv[1]);
	}

	if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		err_quit("socket");

	// in recieve of sigalarm, send seq and timestamp
	signal(SIGALRM, do_send);

	//do_send(SIGALRM);
	
	int file_n = atoi(argv[2]);
	int i=0;
	count = 0;
	while(count<file_n) {
	//while(count<11) {
		int rlen;
		struct sockaddr_in csin;
		socklen_t csinlen = sizeof(csin);
		char buf[2048];
		struct timeval tv;
		ping_t *p = (ping_t *) buf;

		// transfer file
		int temp = count;
		char file_name[7] = "000000";
		for(int j=5;j>=3;j--){
			file_name[j] = (char)('0' + temp%10);
			temp /= 10;
		}
		// define file name
		char* read_file = malloc(128);
		strcpy(read_file, argv[1]);
		strcat(read_file, "/");
		strcat(read_file, file_name);
		printf("FILE NAME: %s\n", read_file);
		FILE* input_file = fopen(read_file, "r");
		if(input_file == NULL){
			perror("fopen");
			break;
		}
		// transfer
		send_file_data(input_file, s, sin);
		
		printf("TARGET FILE===========================\n");
		int c;
		FILE *file = fopen(read_file, "r");
		/*if (file) {
			while ((c = getc(file)) != EOF)
				putchar(c);
			fclose(file);
		}*/
		
	}
	//closedir(dfd);
	close(s);
}