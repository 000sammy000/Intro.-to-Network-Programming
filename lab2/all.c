/*can implement a single command to do all*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h> 
#include <string.h>
#include <arpa/inet.h>

#pragma pack(1) 
typedef struct {
    uint64_t magic;     /* 'BINFLAG\x00' */
    uint32_t datasize;  /* in big-endian */
    uint16_t n_blocks;  /* in big-endian */
    uint16_t zeros;
}binflag_header_t;
#pragma pack()

typedef struct {
    uint32_t offset;        /* in big-endian */
    uint16_t cksum;         /* XOR'ed results of each 2-byte unit in payload */
    uint16_t length;        /* ranges from 1KB - 3KB, in big-endian */
    uint8_t payload[0];
} __attribute__((packed)) block_t;

// Define a struct to store offset and payload
typedef struct {
    uint32_t offset;
    uint8_t *payload;
    size_t payload_length;
} OffsetPayload;

int compareOffsets(const void *a, const void *b) {
    return ((OffsetPayload *)a)->offset - ((OffsetPayload *)b)->offset;
}

typedef struct {
   uint16_t length;        /* length of the offset array, in big-endian */
   uint32_t offset[0];     /* offset of the flags, in big-endian */
} __attribute__((packed)) flag_t;


uint64_t manual_ntohll(uint64_t network_value) {
    return ((uint64_t)ntohl((uint32_t)(network_value & 0xFFFFFFFF)) << 32) | ntohl((uint32_t)(network_value >> 32));
}


int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("Usage: %s <id>\n", argv[0]);
        return 1;
    }

    const char *id = argv[1];  

    
    char command[100]; 
    snprintf(command, sizeof(command), "curl 'https://inp.zoolab.org/binflag/challenge?id=%s' --output mumu.bin", id);

    int result = system(command);


    if (result == 0) {
        printf("curl 執行成功.\n");
    } else {
        printf("curl 執行失敗.\n");
    }
    
    FILE *file;
    binflag_header_t header;
    
    // 尝试打开二进制文件（以二进制只读方式）
    file = fopen("mumu.bin", "rb");

    // 检查文件是否成功打开
    if (file == NULL) {
        printf("無法打開文件.\n");
        return 1;
    }

    size_t bytes_read = fread(&header, sizeof(binflag_header_t), 1, file);

    if (bytes_read != 1) {
        printf("無法讀取header.\n");
        fclose(file);
        return 1;
    }

    /*printf("Data Size: %u\n", ntohl(header.datasize));
    printf("Number of Blocks: %u\n", ntohs(header.n_blocks));
    printf("Zeros: %u\n", ntohs(header.zeros));*/

    //block
    OffsetPayload *offsetPayloadArray = (OffsetPayload *)malloc(sizeof(OffsetPayload) * ntohs(header.n_blocks));
    int good=0;

    for (int i = 0; i < ntohs(header.n_blocks); i++) {
        block_t block;
        size_t block_bytes_read = fread(&block, sizeof(block_t), 1, file);

        if (block_bytes_read != 1) {
            printf("无法读取Block %d.\n", i);
            break;
        }

        /*printf("Offset: ");
        for (size_t j = 0; j < sizeof(block.offset); j++) {
            printf("%02X ", ((uint8_t *)&block.offset)[j]);
        }
        printf("\n");

        printf("Checksum ");
        for (size_t j = 0; j < sizeof(block.cksum); j++) {
            printf("%02X ", ((uint8_t *)&block.cksum)[j]);
        }
        printf("\n");

        printf("Length: %u\n", ntohs(block.length));*/

        uint16_t payload_length = ntohs(block.length);

        // 分配内存
        uint8_t *payload_data = (uint8_t *)malloc(payload_length);
        if (payload_data == NULL) {
            printf("內存分配失敗\n");
            fclose(file);
            return 1;
        }

        size_t payload_bytes_read = fread(payload_data, 1, payload_length, file);

        if (payload_bytes_read != payload_length) {
            printf("無法讀取payload.\n");
            free(payload_data);
            fclose(file);
            return 1;
        }

        /*printf("Payload:\n");
        for (size_t j = 0; j < payload_length; j++) {
            printf("%02X ", payload_data[j]);
            if ((j + 1) % 16 == 0) {
                printf("\n");
            }
        }*/
        

        uint8_t xor_result1=payload_data[0], xor_result2=payload_data[1];
        for (size_t j = 2; j < payload_length; j += 2) {
            xor_result1 = xor_result1 ^ payload_data[j];
            
        }
        for (size_t j = 3; j < payload_length; j += 2) {
            xor_result2 = xor_result2 ^ payload_data[j];
            
        }
        /*printf("XOR Result: ");
        printf("%02X %02X", xor_result1,xor_result2);
        printf("\n");*/

        
        if(((uint8_t *)&block.cksum)[0]==xor_result1&&((uint8_t *)&block.cksum)[1]==xor_result2){
            offsetPayloadArray[good].offset = ntohl(block.offset);
            offsetPayloadArray[good].payload = payload_data;
            offsetPayloadArray[good].payload_length = payload_length;
            good++;
        }
        
        
        //free(payload_data);
          
    }

    qsort(offsetPayloadArray, good, sizeof(OffsetPayload), compareOffsets);

    uint8_t *allPayloads = (uint8_t *)malloc(ntohl(header.datasize));
    size_t currentOffset = 0;

    // Now you have the payloads sorted by offset in offsetPayloadArray
    for (int i = 0; i < good; i++) {
        

        size_t payload_length = offsetPayloadArray[i].payload_length;
        memcpy(allPayloads + currentOffset, offsetPayloadArray[i].payload, payload_length);
        currentOffset += payload_length;

        // Free payload data
        free(offsetPayloadArray[i].payload);
    }

    

    // Free the offsetPayloadArray
    free(offsetPayloadArray);

    

    flag_t flag;
    size_t flag_bytes_read = fread(&flag, sizeof(uint16_t), 1, file);

    if (flag_bytes_read != 1) {
        printf("無法讀取flag.\n");
        fclose(file);
        return 1;
    }

    
    uint16_t flag_length = ntohs(flag.length);
   //printf("Flag Length: %u\n", flag_length);
    


    //讀offset

    char offsetString[1024]; // Adjust the size as needed
    offsetString[0] = '\0'; // Ensure the string is empty initiall
    for (int i = 0; i < flag_length; i++) {
        uint32_t offset;
        size_t offset_bytes_read = fread(&offset, sizeof(uint32_t), 1, file);

        if (offset_bytes_read != 1) {
            printf("無法讀offset %d.\n", i);
            break;
        }


        char temp[5]; // To store the formatted values
        snprintf(temp, sizeof(temp), "%02X%02X", allPayloads[ntohl(offset)], allPayloads[ntohl(offset) + 1]);
        strcat(offsetString, temp);
    }
    
    printf("\n");
    printf("flag: %s\n", offsetString);
    free(allPayloads);

    char url[1000];
    snprintf(url, sizeof(url), "curl 'https://inp.zoolab.org/binflag/verify?v=%s'", offsetString);
    result = system(url);

    if (result == 0) {
        printf("curl 執行成功.\n");
    } else {
        printf("curl 執行失敗.\n");
    }
    
    // 關閉文件
    fclose(file);

    return 0;
}
