#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <openssl/sha.h>
#include <stdbool.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUF_SIZE 1024

void compute_SHA256(unsigned char* hash, unsigned char* data, size_t length) {
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data, length);
    SHA256_Final(hash, &sha256);
}

// 16진수 한 자리를 나타내는 비트가 0인지 확인
bool check_4bits(unsigned char byte, int position) {
    return (byte & (0xF << position)) == 0;
}

// 처음부터 difficulty 수만큼의 비트(16진수로 변환했을 때)가 모두 0인지 검사
int is_valid(unsigned char* hash, int difficulty) {
    int full_bytes = difficulty / 2;
    int extra_bits = difficulty % 2;

    for(int i=0; i < full_bytes; i++) {
        if(hash[i] != 0) {
            return 0;
        }
    }

    if(extra_bits > 0 && !check_4bits(hash[full_bytes], 4)) {
        return 0;
    }

    return 1;
}

void print_hash(unsigned char* hash) {
    for(int i=0; i < SHA256_DIGEST_LENGTH; i++) {
        printf("%02x", hash[i]);
    }
    printf("\n");
}

int main()
{
    int sock;
    struct sockaddr_in server_addr;
    char msg[BUF_SIZE];
    int msg_len;
    int challenge, difficulty, nonce = 0;

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("socket() error\n");
        exit(0);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(PORT);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        printf("connect() error\n");
        exit(0);
    }
    else
        printf("Connected to main server\n");

    // Main Server로부터 학번(challenge), 난이도 수신
    msg_len = read(sock, msg, BUF_SIZE-1);
    msg[msg_len] = '\0';
    sscanf(msg, "%d %d", &challenge, &difficulty);  // 문자열로부터 학번과 난이도 추출
    printf("Challenge: %d, Difficulty: %d\n", challenge, difficulty);

    // PoW(작업증명) 시작
    unsigned char text[64];
    unsigned char hash[SHA256_DIGEST_LENGTH];
    while(1) {
        sprintf(text, "%d%d", challenge, nonce);
        compute_SHA256(hash, text, strlen(text));
        print_hash(hash);
        if(is_valid(hash, difficulty)) {
            sprintf(msg, "Success! Nonce: %d\n", nonce);
            printf("Success! Nonce: %d\n", nonce);
            write(sock, msg, sizeof(msg));  // 결과를 Main Server에게 전송
            break;
        }
        nonce++;
    }

    close(sock);

    return 0;
}