#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8085

int main() {
    struct sockaddr_in serverAddress;
    int sock = 0;
    char* message = "Message from client";
    char buffer[1024] = { 0 };

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("create error \n");
        exit(EXIT_FAILURE);
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);

    // Преобразование адреса в двоичную форму
    if (inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) <= 0) {
        printf("Error: Address not supported \n");
        exit(EXIT_FAILURE);
    }

    //connect
    if (connect(sock, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        printf("Connection failed \n");
        exit(EXIT_FAILURE);
    }

    send(sock, message, strlen(message), 0);
    printf("Message sent \n");

    close(sock);

    return 0;
}