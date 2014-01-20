//
//  main.cpp
//  WebServerFirst
//
//  Created by Pavlo Kytsmey on 1/17/14.
//  Copyright (c) 2014 Pavlo Kytsmey. All rights reserved.
//

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <list>
#include <vector>

#include "Queue.h"

using namespace std;

#define BACKLOG 10

struct Task {
    int sock; // socket where to send
    std::vector<char> recvMsg; // received message
    Task(int s, std::vector<char> msg);
};

Task::Task(int s, std::vector<char> msg){
    sock = s;
    recvMsg = msg;
}

struct Queue<Task> taskList; // Queue that keeps all tasks

void* doTask(void* q){
    char beginMessage[] = "HTTP/1.1 200 Okay\r\nContent-Type: text/html; charset=ISO-8859-4 \r\n\r\n<h1>";
    char endMessage[] = "</h1>";
    while(true){
        Task t = taskList.pop();
        if(t.recvMsg.size() != 0){
            if (send(t.sock, beginMessage, strlen(beginMessage), 0) == -1){
                perror("send");
            }
            if (send(t.sock, &t.recvMsg[0], t.recvMsg.size(), 0) == -1)
                perror("send");
            if (send(t.sock, endMessage, strlen(endMessage), 0) == -1)
                perror("send");
            close(t.sock);
            cout << "message sent" << endl;
        }else{
            char msg[] = "HTTP/1.1 200 Okay\r\nContent-Type: text/html; charset=ISO-8859-4 \r\n\r\n<h1>WebServer responded!!!</h1>";
            // since no message exists we send the standard one
            unsigned long size = strlen(msg);
            if (send(t.sock, msg, size, 0) == -1)
                perror("send");
            close(t.sock);
            cout << "message sent" << endl;
        }
    }
    return NULL;
}

int main(int argc, const char * argv[])
{
    std::cout << "Web Server Started.\n";
    
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    struct addrinfo hints;
    struct addrinfo *res;
    int status, sockfd;
    
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    status = getaddrinfo(NULL, "3490", &hints, &res);
    if (status != 0){
        std::cerr << "ERROR: unsuccessful getaddrinfo return" << std::endl;
        exit(1);
    }
    
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1){
        cerr << "ERROR: socket() failed\n";
        exit(1);
    }
    if(::bind(sockfd, res->ai_addr, res->ai_addrlen) == -1){
        close(sockfd);
        perror("server: bind");
        exit(1);
    }
    
    freeaddrinfo(res);
    
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    int numConsumers = 20;
    pthread_t consumers[numConsumers];
    
    for(int i = 0; i < numConsumers; i++){
        // creating consumer's thread that will respond to requests
        pthread_create(&consumers[i], NULL, &doTask, NULL);
    }
    
    while (true) {
        addr_size = sizeof their_addr;
        int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
        if (new_fd == -1){
            cerr << "ERROR: by accept()\n";
            exit(1);
        }
        int sizeRecvMessage = 1024;
        char recvMessage[sizeRecvMessage];
        long len = recv(new_fd, recvMessage, sizeRecvMessage, 0);
        if(len == -1){
            free(recvMessage);
            cerr << "ERROR: failed on receiving" << endl;
            exit(1);
        }
        vector<char> recvMsg (recvMessage, recvMessage+len*sizeof(char));
        Task tsk (new_fd, recvMsg);
        taskList.push(tsk);
    }
    
    return 0;
}

