#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#define PORT 8080
#define BUF_SIZE 1024

int main()
{
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_size;
    
    char msg[BUF_SIZE];
    int msg_len;
    
    struct timeval start, end;

    server_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (server_sock == -1)
        printf("socket() error\n");
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
        printf("bind() error\n");
    if (listen(server_sock, 5) == -1)
        printf("listen() error\n");
    
    client_addr_size = sizeof(client_addr);

    // FIXME: 일단 클라이언트 하나만 됨. 여러개 되게 수정 ㄱㄱ
    client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_size);
    if (client_sock == -1)
        printf("accept() error\n");
    else
        printf("Client connected\n");
	
    // 학번(challenge), 난이도를 Working Server에게 전송
    sprintf(msg, "%d %d", 20192847, 5);
    gettimeofday(&start, NULL);
    write(client_sock, msg, sizeof(msg));

    // Working Server로부터 PoW 결과를 수신
    msg_len = read(client_sock, msg, BUF_SIZE-1);
    gettimeofday(&end, NULL);
    msg[msg_len] = '\0';
    printf("* Message from client: %s", msg);

    double time_elapsed = ((double) end.tv_sec - start.tv_sec) + ((double) end.tv_usec - start.tv_usec) * 1e-6;
    printf("* Time for PoW: %f seconds\n", time_elapsed);
	
    close(client_sock);
    close(server_sock);

    return 0;
}