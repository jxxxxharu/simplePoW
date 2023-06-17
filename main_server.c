#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <limits.h>

#define PORT 1255
#define BUF_SIZE 1024
#define MAX_CLIENTS 2

int main()
{
    int server_sock, client_socks[MAX_CLIENTS];
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_size;
    
    char msg[BUF_SIZE];
    int msg_len;
    long start_num[MAX_CLIENTS] = {0, LONG_MAX / MAX_CLIENTS};  // 각 Working Server에게 할당된 nonce 범위 시작값
    int i, max_sd, fd_num;
    fd_set read_fds, cpy_fds;

    struct timeval start, end;

    char challenge[BUF_SIZE];
    int difficulty;
    printf("Enter Challenge>> ");
    scanf("%s", challenge);  // 학번 입력
    printf("Enter Difficulty>> ");
    scanf("%d", &difficulty);  // 난이도 입력

    server_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        printf("socket() error\n");
        exit(1);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        printf("bind() error\n");
        exit(1);
    }
    if (listen(server_sock, MAX_CLIENTS) == -1) {
        printf("listen() error\n");
        exit(1);
    }
    
    client_addr_size = sizeof(client_addr);

    for (i=0; i < MAX_CLIENTS; i++) {
        client_addr_size = sizeof(client_addr);
        client_socks[i] = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_size);
        if (client_socks[i] == -1) {
            printf("accept() error\n");
            exit(1);
        }
        printf("Client %d connected\n", i);

        // Challenge, Difficulty, start_nonce를 문자열로 변환하여 메시지 구성 후 Working Server에게 전송
        sprintf(msg, "%s %d %ld", challenge, difficulty, start_num[i]);
        gettimeofday(&start, NULL);
        write(client_socks[i], msg, sizeof(msg));
    }

    FD_ZERO(&read_fds);
    FD_SET(server_sock, &read_fds);
    for (i=0; i < MAX_CLIENTS; i++) {
        if (client_socks[i] > max_sd) {
            max_sd = client_socks[i];
        }
        FD_SET(client_socks[i], &read_fds);
    }

    cpy_fds = read_fds; // select() 호출 전 read_fds 복사본 만들어둠
    fd_num = select(max_sd+1, &cpy_fds, NULL, NULL, NULL);
    if (fd_num <= 0) {
        printf("select() error\n");
        exit(1);
    }

    for (i=0; i < max_sd+1; i++) {
        if (FD_ISSET(client_socks[i], &cpy_fds)) {
            // Working Server로부터 PoW 결과 수신
            msg_len = read(client_socks[i], msg, BUF_SIZE-1);
            gettimeofday(&end, NULL);
            msg[msg_len] = '\0';
            printf("* Message from client %d: %s", i, msg);

            double time_elapsed = ((double)end.tv_sec - start.tv_sec) + ((double)end.tv_usec - start.tv_usec) * 1e-6;
            printf("* Time for PoW: %f seconds\n", time_elapsed);

            // nonce를 찾은 뒤, 다른 Working Server에게 스톱 싸인 전송
            for (int j = 0; j < MAX_CLIENTS; j++) {
                if (j != i) {
                    write(client_socks[j], "STOP", sizeof("STOP"));
                }
            }

            close(client_socks[i]);
            FD_CLR(client_socks[i], &read_fds);
            break;
        }
    }

    for (i=0; i < MAX_CLIENTS; i++) {
        if (client_socks[i] != -1)
            close(client_socks[i]);
            FD_CLR(client_socks[i], &read_fds);
    }

    close(server_sock);

    return 0;
}