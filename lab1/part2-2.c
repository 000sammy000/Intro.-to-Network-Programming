#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>

int begin, end;
int packet_lengths[500]; // 用于存储数据包长度的数组

void packet_handler(const struct pcap_pkthdr *pkthdr, const unsigned char *packet) {
    int seq = 0;
    for (int i = 0; i < pkthdr->len; i++) {
        if (packet[i] == '/') {
            seq = 100 * (packet[i+3] - '0') + 10 * (packet[i+4] - '0') + (packet[i+5] - '0');
        }
        
        if (packet[i] == ':') {
            if (packet[i+1] == 'B') {
                //printf("begin=%d\n", seq);
                begin = seq;
            } else if (packet[i+1] == 'E') {
                //printf("end=%d\n", seq);
                end = seq;
            }
        }
    }
    // 存储数据包长度到数组中
    packet_lengths[seq] = pkthdr->len;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <pcap_file>\n", argv[0]);
        return 1;
    }

    char *pcap_file = argv[1];
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle;

    // 打开 pcap 文件
    handle = pcap_open_offline(pcap_file, errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Error opening pcap file: %s\n", errbuf);
        return 1;
    }

    const unsigned char *packet;
    struct pcap_pkthdr header;

    // 初始化packet_lengths数组为-1，表示还没有收到这个序列的数据包
    for (int i = 0; i < 500; i++) {
        packet_lengths[i] = -1;
    }

    // 读取下一个数据包，直到文件结束
    while ((packet = pcap_next(handle, &header)) != NULL) {
        packet_handler(&header, packet);
    }
    printf("begin=%d\n", begin);
    printf("begin=%d\n", end);
    // 打印存储的数据包长度
    for (int i = begin+1; i < end; i++) {
        if (packet_lengths[i] != -1) {
            printf("%c", packet_lengths[i]-48);
        }
    }
    printf("\n");
    pcap_close(handle);
    return 0;
}
