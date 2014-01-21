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
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>

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

struct Queue<int> taskList; // Queue that keeps all tasks

void* doTask(void* q){
    char beginMessage[] = "HTTP/1.1 200 Okay\r\nContent-Type: text/html; charset=ISO-8859-4 \r\n\r\n<h1>";
    char endMessage[] = "</h1>";
    while(true){
        int sock = taskList.pop();
        int sizeRecvMessage = 1024;
        char recvMessage[sizeRecvMessage];
        long len = recv(sock, recvMessage, sizeRecvMessage, 0);
        if(len == -1){
            cerr << "ERROR: failed on receiving" << endl;
            exit(1);
        }
        if(len != 0){
            
            
            std::string fileName("/Users/pkyt/Desktop/github/WebServer/WebServerFirst/send_file.txt");
            string headerMsg = "HTTP/1.1 200 Okay\r\nContent-Type: text/txt;\r\nContent-disposition: attachment; filename=send_file.txt\r\n\r\n";
            send(sock, &(headerMsg[0]), headerMsg.length(), 0);
            FILE * fp = fopen(&(fileName[0]), "r");
            struct stat fileStatus;
            stat(&(fileName[0]), &fileStatus);
            long fileSize = fileStatus.st_size;
            long sizeCheck = 0;
            char mfcc[1025];
            while (sizeCheck + 1024 <= fileSize){
                size_t read = ::fread(mfcc, sizeof(char), 1024, fp);
                long sent = send(sock, mfcc, read, 0);
                sizeCheck += sent;
            }
            if(sizeCheck > 0){
                char mfccPart [fileSize - sizeCheck + 1];
                ::fread(mfccPart, sizeof(char), fileSize - sizeCheck, fp);
                send(sock, mfcc, fileSize, 0);
            }
            fclose(fp);
            close(sock);
            cout << "message sent" << endl;
        }else{
            char msg[] = "HTTP/1.1 200 Okay\r\nContent-Type: text/html; charset=ISO-8859-4 \r\n\r\n<h1>WebServer responded!!!</h1>";
            // since no message exists we send the standard one
            unsigned long sizeMsg = strlen(msg);
            if (send(sock, msg, sizeMsg, 0) == -1)
                perror("send");
            close(sock);
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
        taskList.push(new_fd);
    }
    
    return 0;
}

