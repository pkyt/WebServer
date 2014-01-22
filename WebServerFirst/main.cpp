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

void sendError(int sock){
    string err404 = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\nERROR 404: Not Found";
    if (send(sock, &(err404[0]), err404.length(), 0) == -1)
        perror("send");
}

void sendStandardMsg(int sock){
    char msg[] = "HTTP/1.1 200 Okay\r\nContent-Type: text/html; charset=ISO-8859-4 \r\n\r\n<h1>WebServer responded!!!</h1>";
    unsigned long sizeMsg = strlen(msg);
    if (send(sock, msg, sizeMsg, 0) == -1)
        perror("send");
}

bool checkEmpty(string s){
    string hasToBe = "HTTP/1.";
    for (int i = 0; i < hasToBe.length(); i++){
        if(hasToBe[i] != s[i]){
            return false;
        }
    }
    return true;
}

void* doTask(void* q){
    while(true){
        
        int sock = taskList.pop();
        cout << "message recieved from socket " << sock << endl;
        vector<char>recvMsg;
        int sizeRecvMessage = 1024;
        long len = sizeRecvMessage;
        while(len == 1024){
            char recvMessage[sizeRecvMessage];
            len = recv(sock, recvMessage, sizeRecvMessage, 0);
            recvMsg.insert(recvMsg.end(), recvMessage, recvMessage+len);
            if(len == -1){
                cerr << "ERROR: failed on receiving" << endl;
                exit(1);
            }
        }
        if(len != 0){
            char * pch;
            pch = strtok(&(recvMsg[0]), "/");
            pch = strtok(NULL, " \n"); // Now pch correspond to the specidic data (path to data) a client needs
            if (pch == NULL){ // if no data needed send standard message
                sendStandardMsg(sock);
            }else{
                cout << pch << endl;
                if (checkEmpty(pch)){ // no file requested
                    sendStandardMsg(sock);
                }else{
                    string beginPath = "/Users/pkyt/Desktop/github/WebServer/WebServerFirst/";
                    string restPath = pch;
                    string fileName;
                    pch = strtok(&(restPath[0]), "/");
                    while (pch != NULL){
                        fileName = pch;
                        pch = strtok(NULL, "/");
                    }
                    string path = beginPath + restPath;
                    struct stat fileStatus;
                    stat(&(path[0]), &fileStatus);
                    if (fileStatus.st_size == 0){ // file doesn't exist, ssince size o file  == 0
                        sendError(sock);
                    }else{ // file exists
                        string headerMsg = "HTTP/1.1 200 Okay\r\nContent-disposition: attachment; filename=" + fileName + "\r\nContent-type: ";
                        string type; // type of file
                        pch = strtok(&(fileName[0]), ".");
                        pch = strtok(NULL, ".");
                        if(pch == NULL){ // this case should rarely happen
                            type = "txt";
                        }else{
                            type = pch;
                        }
                        string contentType;
                        if (type == "jpeg"){
                            contentType = "image/jpeg;";
                        }else{
                            contentType = "text/txs;";
                        }
                        headerMsg = headerMsg + contentType + "\r\n\r\n";
                        send(sock, &(headerMsg[0]), headerMsg.length(), 0);
                        FILE * fp = fopen(&(path[0]), "r");
                        long fileSize = fileStatus.st_size;
                        long sizeCheck = 0;
                        char mfcc[1025];
                        while (sizeCheck + 1024 <= fileSize){ // sending data by 1024 bytes
                            size_t read = ::fread(mfcc, sizeof(char), 1024, fp);
                            long sent = send(sock, mfcc, read, 0);
                            if(sent == -1){
                                perror("sent");
                                break;
                            }
                            sizeCheck += sent;
                        }
                        if(fileSize - sizeCheck > 0){ // send rest of data
                            char mfccPart [fileSize - sizeCheck + 1];
                            ::fread(mfccPart, sizeof(char), fileSize - sizeCheck, fp);
                            long sent = send(sock, mfccPart, fileSize - sizeCheck, 0);
                            if(sent == -1){
                                perror("sent");
                                break;
                            }
                        }
                        fclose(fp);
                    }
                }
            }
        }else{
            sendStandardMsg(sock);
        }
        close(sock);
        cout << "message sent" << endl;
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

