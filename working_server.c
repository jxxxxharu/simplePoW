#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <openssl/sha.h>
#include <stdbool.h>
#include <pthread.h>
#include <limits.h>

#define SERVER_IP "119.192.210.203"
#define PORT 1255
#define BUF_SIZE 1024
#define THREADS_NUM 5

pthread_mutex_t stop_mutex, pow_mutex;
volatile int stop_sign = 0;  // critical section
volatile int found = 0;  // critical section (nonce를 찾았는 지 여부)

int sock;
char challenge[BUF_SIZE];
int difficulty;
long start_num;
// long offset = (LONG_MAX - start_num + 1) / THREADS_NUM;

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

// PoW(작업증명) 쓰레드
void *PoW(void *arg) {
    long tid = (long)arg;
    unsigned char text[64];
    unsigned char hash[SHA256_DIGEST_LENGTH];
    char msg[BUF_SIZE];

    for(long n=(start_num+tid); n <= LONG_MAX; n+=THREADS_NUM) {
        sprintf(text, "%s%ld", challenge, n);
        compute_SHA256(hash, text, strlen(text));
        
        pthread_mutex_lock(&stop_mutex);
        if(stop_sign) {
            pthread_mutex_unlock(&stop_mutex);
            pthread_exit(0);
        }
        pthread_mutex_unlock(&stop_mutex);
        pthread_mutex_lock(&pow_mutex);
        if(found) {
            pthread_mutex_unlock(&pow_mutex);
            pthread_exit(0);
        }
        pthread_mutex_unlock(&pow_mutex);

        if(is_valid(hash, difficulty)) {
            pthread_mutex_lock(&pow_mutex);
            if(!found) {
                found = 1;
                pthread_mutex_unlock(&pow_mutex);
                printf("\n");
                print_hash(hash);
                sprintf(msg, "Success! Nonce: %ld\n", n);
                printf("%s", msg);
                printf("Among #0~#%d threads, thread #%ld found the nonce!!\n", THREADS_NUM-1, tid);
                write(sock, msg, strlen(msg)+1);
                pthread_exit(0);
            }
            pthread_mutex_unlock(&pow_mutex);
        }
    }
    return NULL;
}

// 스톱 싸인 수신 쓰레드
void *listen_stop_sign(void *socket) {
    int sock = *(int*)socket;
    char buf[BUF_SIZE];
    
    while(1) {
        int buf_len = read(sock, buf, BUF_SIZE-1);
        if(buf_len > 0) {
            if(strcmp(buf, "STOP") == 0) {
                pthread_mutex_lock(&stop_mutex);
                stop_sign = 1;
                pthread_mutex_unlock(&stop_mutex);
                printf("\nComputation stopped due to stop sign.\n");
                break;
            }
        }
    }
    return NULL;
}

int main()
{
    struct sockaddr_in server_addr;
    char msg[BUF_SIZE];
    int msg_len;
    
    pthread_t threads[THREADS_NUM];
    pthread_t stop_thread;
    pthread_mutex_init(&stop_mutex, NULL);
    pthread_mutex_init(&pow_mutex, NULL);

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("socket() error\n");
        exit(1);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(PORT);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        printf("connect() error\n");
        exit(1);
    }
    else
        printf("Connected to main server\n");

    // Main Server로부터 학번(challenge), 난이도 수신
    msg_len = read(sock, msg, BUF_SIZE-1);
    msg[msg_len] = '\0';
    sscanf(msg, "%s %d %ld", challenge, &difficulty, &start_num);  // 문자열로부터 학번과 난이도 추출
    printf("Challenge: %s, Difficulty: %d, start_nonce: %ld\n", challenge, difficulty, start_num);

    for(long tid=0; tid < THREADS_NUM; tid++) {
        pthread_create(&threads[tid], NULL, PoW, (void *)tid);
    }
    pthread_create(&stop_thread, NULL, listen_stop_sign, (void *)&sock);

    for(int i=0; i < THREADS_NUM; i++) {
        pthread_join(threads[i], NULL);
    }
    pthread_mutex_destroy(&stop_mutex);
    pthread_mutex_destroy(&pow_mutex);
    close(sock);

    return 0;
}