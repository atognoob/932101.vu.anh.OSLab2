#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#define PORT 8085   //Указывает порт, который будет прослушивать сервер
#define BACKLOG 5   //Максимальное количество соединений, ожидающих принятия.

//Объявление обработчика сигнала
volatile sig_atomic_t wasSigHup = 0;    //wasSigHup используется для обозначения того, был ли получен сигнал SIGHUP или нет.

//Функция обрабатывает сигнал SIGHUP, когда он отправляется в программу.
void sigHupHandler(int sigNumber) {
    wasSigHup = 1;
}

int main() {
    int serverFD;   //представляет файловый дескриптор сервера сокетов.
    int incomingSocketFD = 0; //представляет файловый дескриптор соединения с принимаемым сокетом.
    struct sockaddr_in socketAddress;   
    int addressLength = sizeof(socketAddress);
    fd_set readfds;
    struct sigaction sa;
    sigset_t blockedMask, origMask;
    char buffer[1024] = { 0 };
    int readBytes;
    int maxSd;
    int signalOrConnectionCount = 0;

    // создаем сокет
    if ((serverFD = socket(AF_INET, SOCK_STREAM, 0)) == 0) {        //IPv4/TCP
        perror("create error");
        exit(EXIT_FAILURE);
    }

    // настройки адреса сокета
    socketAddress.sin_family = AF_INET;
    socketAddress.sin_addr.s_addr = INADDR_ANY;
    socketAddress.sin_port = htons(PORT);

    // привязываем сокет к адресу
    if (bind(serverFD, (struct sockaddr*)&socketAddress, sizeof(socketAddress)) < 0) {
        perror("bind error");
        exit(EXIT_FAILURE);
    }

    if (listen(serverFD, BACKLOG) < 0) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }

    printf("Server started on port %d \n", PORT);

    // Регистрация обработчика сигнала
    sigaction(SIGHUP, NULL, &sa);
    sa.sa_handler = sigHupHandler;
    sa.sa_flags |= SA_RESTART;
    sigaction(SIGHUP, &sa, NULL);

    // блокирование сигнала
    sigemptyset(&blockedMask);
    sigemptyset(&origMask);
    sigaddset(&blockedMask, SIGHUP);
    sigprocmask(SIG_BLOCK, &blockedMask, &origMask);
   
    //Работа основного цикла
    while (signalOrConnectionCount < 3) {
        FD_ZERO(&readfds); 
        FD_SET(serverFD, &readfds); 
        
        if (incomingSocketFD > 0) { 
            FD_SET(incomingSocketFD, &readfds); 
        } 
        
        maxSd = (incomingSocketFD > serverFD) ? incomingSocketFD : serverFD; 
 
        if (pselect(maxSd + 1, &readfds, NULL, NULL, NULL, &origMask) == -1) { 
        if (errno != EINTR) {
            perror("pselect error"); 
            exit(EXIT_FAILURE); 
            }
        if (errno == EINTR){
            // Получение проверки сигнала SIGHUP
        	if(wasSigHup==1){
        		printf("SIGHUP received.\n");
  			    wasSigHup = 0;
            	signalOrConnectionCount++;
                continue;
        	}
        }
        }

        // Чтение входящих байтов
        if (incomingSocketFD > 0 && FD_ISSET(incomingSocketFD, &readfds)) { 
            readBytes = read(incomingSocketFD, buffer, 1024);

            if (readBytes > 0) { 
                printf("Received data: %d bytes\n", readBytes); 
            } else {
                if (readBytes == 0) {
                    close(incomingSocketFD); 
                    incomingSocketFD = 0; 
                } else { 
                    perror("read error"); 
                } 
                signalOrConnectionCount++;   
            } 
            continue;
        }
        
        // Проверка входящих соединений
        if (FD_ISSET(serverFD, &readfds)) {
            if ((incomingSocketFD = accept(serverFD, (struct sockaddr*)&socketAddress, (socklen_t*)&addressLength)) < 0) {
                perror("accept error");
                exit(EXIT_FAILURE);
            }

            printf("New connection.\n");
        }
    }

    close(serverFD);

    return 0;
}