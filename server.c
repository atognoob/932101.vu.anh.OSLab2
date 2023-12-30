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
    struct sigaction sa;
    sigset_t blockedMask, origMask;
    

    // создаем сокет
    if ((serverFD = socket(AF_INET, SOCK_STREAM, 0)) == 0) {        //IPv4/TCP
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // настройки адреса сокета
    struct sockaddr_in socketAddress;   
    socketAddress.sin_family = AF_INET;
    socketAddress.sin_addr.s_addr = INADDR_ANY;
    socketAddress.sin_port = htons(PORT);
    int addressLength = sizeof(socketAddress);
    // привязываем сокет к адресу
    if (bind(serverFD, (struct sockaddr*)&socketAddress, sizeof(socketAddress)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(serverFD, BACKLOG) == -1) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d \n", PORT);

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
   
    fd_set readfds;
    int countConnection = 0;
    int maxSd;
    int socket2 = 0; //представляет файловый дескриптор соединения с принимаемым сокетом.
   
    //Работа основного цикла
    while (countConnection < 5) {
        FD_ZERO(&readfds); 
        FD_SET(serverFD, &readfds); 
        
        if (socket2 > 0) { 
            FD_SET(socket2, &readfds); 
        } 
        
        maxSd = (socket2 > serverFD) ? socket2 : serverFD; 

        //вызов pselect
        if (pselect(maxSd + 1, &readfds, NULL, NULL, NULL, &origMask) == -1) { 
            if (errno != EINTR) {
                perror("pselect error"); 
                exit(EXIT_FAILURE); 
            }
            if (errno == EINTR){
            // Получение проверки сигнала SIGHUP
        	    if(wasSigHup){
        		    printf("SIGHUP signal received.\n");
  			        wasSigHup = 0;
            	    countConnection++;
                    continue;
        	    }
            }
        }

        char buffer[1024] = { 0 };
        int readBytes;
        // Чтение входящих байтов
        if (socket2 > 0 && FD_ISSET(socket2, &readfds)) { 
            readBytes = read(socket2, buffer, 1024);

            if (readBytes > 0) { 
                printf("Received data: %d bytes\n", readBytes); 
            } else {
                if (readBytes == 0) {
                    close(socket2); 
                    socket2 = 0; 
                } else { 
                    perror("read error"); 
                } 
                countConnection++;   
            } 
            continue;
        }
        
        // Проверка входящих соединений
        if (FD_ISSET(serverFD, &readfds)) {
            if ((socket2 = accept(serverFD, (struct sockaddr*)&socketAddress, (socklen_t*)&addressLength)) < 0) {
                perror("accept error");
                exit(EXIT_FAILURE);
            }

            printf("New connection.\n");
        }
    }

    close(serverFD);

    return 0;
}