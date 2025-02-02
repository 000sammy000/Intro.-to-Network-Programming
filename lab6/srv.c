/*
 * Lab problem set for INP course
 * by Chun-Ying Huang <chuang@cs.nctu.edu.tw>
 * License: GPLv2
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define err_quit(m) { perror(m); exit(-1); }

#define SIZE 1024
unsigned int count = 0;
unsigned int written = 0; // sequence that has been written to file
char* read_dir;

typedef struct {
	unsigned file_num;
	unsigned seq;
	char buf[SIZE];
	int total_size;
}	packet_t;


void write_file(int sockfd, struct sockaddr_in addr)
{
	written = 0;
	// client addr
	struct sockaddr_in csin;
	socklen_t csinlen = sizeof(csin);
	// para
	int n;
	char buffer[SIZE];
	socklen_t addr_size;
	// 000000 -> file name
	int temp = count;
	char file_name[7] = "000000";
	for(int j=5;j>=3;j--){
		file_name[j] = (char)('0' + temp%10);
		temp /= 10;
	}
	// define file name
	char* read_file = malloc(128);
	strcpy(read_file, read_dir);
	strcat(read_file, "/");
	strcat(read_file, file_name);
	// Creating a file.
	FILE* fp = fopen(read_file, "w");

	// Receiving the data and writing it into the file.
	long get_size;
	char packet_buf[SIZE*2];
	packet_t *size_packet = (packet_t *) packet_buf;
	
	/*while(n=recvfrom(sockfd, packet_buf, sizeof(packet_buf), 0, (struct sockaddr*) &csin, &csinlen)<0||size_packet->seq!=-1){}
	get_size=size_packet->total_size;
	printf("get size=%ld\n",get_size);
	sendto(sockfd, &size_packet->seq, sizeof(int)  , 0, (struct sockaddr*) &addr, addr_size);*/
	int total=0;
	while (1)
	{
		
		
		addr_size = sizeof(addr);
		//char packet_buf[SIZE*2];
		packet_t *p = (packet_t *) packet_buf;
		//memset(packet_buf, 0, sizeof(packet_buf));
		n = recvfrom(sockfd, packet_buf, sizeof(packet_buf), 0, (struct sockaddr*)&addr, &addr_size);

        get_size=size_packet->total_size;
        //printf("server getsize=%d\n",get_size);

		//snprintf(seq_str, 5, "%d", p->seq);
		char ack[10]="ok";
		
		
		printf("count=%d\n",count);
		if (strcmp(p->buf, "END") == 0)
		{	
			sendto(sockfd, ack,sizeof(ack) , 0, (struct sockaddr*) &addr, addr_size);
			count++;
			break;
		}
		printf("RECV seq: %d, Write=%d\n", p->seq,written);
		if(p->seq == written&&total+SIZE>get_size){
			//printf("[SERVER-RECEVING] Data: %s\n",p->buf);
			printf("[SERVER-RECEVING] Data\n");
			long last_write=get_size-total;
			fwrite(p->buf, 1, last_write,fp);
			count++;
			written++;
			break;
			
		}
		sendto(sockfd, &p->seq,sizeof(int) , 0, (struct sockaddr*) &addr, addr_size);
		
		if(p->seq==0){
				printf("file num=%d\n",p->file_num);
		}
		if(p->seq == written&&p->file_num==count){
			//printf("[SERVER-RECEVING] Data: %s\n",p->buf);
			printf("[SERVER-RECEVING] Data\n");
			fwrite(p->buf, 1, sizeof(p->buf), fp);
			written++;
			total+=SIZE;
		}
		bzero(p->buf, SIZE);
	}

	fclose(fp);
	printf("WRITTEN FILE =======================\n");
	int c;
	FILE *file = fopen(read_file, "r");
	fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
	printf("(server)get size=%ld\n",get_size);
	printf("(server)file=%s file size=%ld\n",read_file,file_size);
	rewind(fp);
	
	/*if (file) {
			while ((c = getc(file)) != EOF)
				putchar(c);
			fclose(file);
	}*/
	
}


int main(int argc, char *argv[]) {
	int s;
	struct sockaddr_in sin;

	if(argc < 2) {
		return -fprintf(stderr, "usage: %s ... <port>\n", argv[0]);
	}
	read_dir = malloc(64);
	strcpy(read_dir, argv[1]);

	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(strtol(argv[argc-1], NULL, 0));

	if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		err_quit("socket");

	if(bind(s, (struct sockaddr*) &sin, sizeof(sin)) < 0)
		err_quit("bind");

	int file_n = atoi(argv[2]);
	for(int i=0;i<file_n;i++)
		write_file(s, sin);
	
	
	
	/*
	while(1) {
		struct sockaddr_in csin;
		socklen_t csinlen = sizeof(csin);
		char buf[2048];
		int rlen;
		
		if((rlen = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr*) &csin, &csinlen)) < 0) {
			perror("recvfrom");
			break;
		}

		sendto(s, buf, rlen, 0, (struct sockaddr*) &csin, sizeof(csin));
	}
	*/

	close(s);
}