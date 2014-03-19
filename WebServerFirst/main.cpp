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
#include "HTTPContentToSend.h"
#include "HTTPDirector.h"
#include "Contacts.h"

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

void sendFile(int sock,string fileName,long fileSize){
    FILE * fp = fopen(&(fileName[0]), "r");
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
        }
    }
    fclose(fp);
}

Contacts conts;

void registration(int sock, string nickName){
    string response = conts.pushContact(sock, nickName);
    if (!strcmp(&(response[0]), "nickName already exists")){
        if (send(sock, &(response[0]), response.length(), 0))
            perror("send");
    }
    cout << &(response[0]) << endl;
}

void sendErrorMessage(int sock, string whatToSend){
    if (send(sock, &(whatToSend[0]), whatToSend.length(), 0))
        perror("send");
}

void handleMessage(int sock, vector<char> msg){
    if (msg.size() < 4) {
        sendErrorMessage(sock,"invalid request");
    }else{
        string command (msg.begin(), msg.begin()+4);
        cout << "1 " << command << endl;
        if (!strcmp(&(command[0]), "Iam:")) {
            string nickName (msg.begin() + 4, msg.end());
            cout << nickName << endl;
            registration(sock, nickName);
        }else if (!strcmp(&(command[0]), "exs:")){
            string nickName (msg.begin() + 4, msg.end());
            cout << nickName << endl;
            string response;
            if (conts.getSocketID(nickName) != -1){
                response = "exs:true";
            }else{
                response = "exs:false";
            }
            cout << response << endl;
            long sent = send(sock, &(response[0]), response.length(), 0);
            
            if (sent == -1) {
                perror("send");
            }
            cout << "sent = " << sent << endl;
        }else if (!strcmp(&(command[0]), "snd:")){
            char* pch = strtok(&(msg[0]), "\n");
            string first (pch);
            pch = strtok(NULL, "\n");
            if (pch == NULL)
                sendErrorMessage(sock,"snd has to have at least 3 lines");
            string toWhom (pch);
            pch = strtok(NULL, "");
            if (pch == NULL)
                sendErrorMessage(sock,"snd has to have at least 3 lines");
            string message (pch);
            string fromWhom (first.begin()+4, first.end());
            int socketToWhom = conts.getSocketID(toWhom);
            
            cout << toWhom << " " << fromWhom << " " << socketToWhom << " " << sock << endl;
            
            
            string parsedMessage = "snd:" + fromWhom + "\n" + message;
            if (send(socketToWhom, &(parsedMessage[0]), parsedMessage.length(), 0))
                perror("send");
        }else if (!strcmp(&(command[0]), "frd:")){
            string allusers = "frd:";
            vector<string> nickNames = conts.getAllUsers();
            for (int i = 0; i < nickNames.size(); i++){
                allusers = allusers  + "\n" + nickNames[i];
            }
            if (send(sock, &(allusers[0]), allusers.length(), 0))
                perror("send");
        }
        
        
    }
    
    /*
    vector<string> allUsers = conts.getAllUsers();
    for (int i = 0; i < allUsers.size(); i++) {
        cout << allUsers[i] << endl;
    }*/
    
    
}

void* doTask(void* q){
    while (true) {
    int sock = taskList.pop();
    while(true){
        
        
        cout << "message recieved from socket " << sock << endl << endl << endl;
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
        
        if (recvMsg.empty()) {
            break;
        }
        cout << &(recvMsg[0]) << endl;
        handleMessage(sock, recvMsg);
        /*
        class HTTPContent wts;
        class HTTPDirector director = HTTPDirector();
        
        bool fileExists = false;
        struct stat fileStatus;
        string path;
        
        
        
        if(len != 0){
            char * pch;
            pch = strtok(&(recvMsg[0]), "/");
            pch = strtok(NULL, " \n"); // Now pch correspond to the specidic data (path to data) a client needs
            if (pch != NULL){
                std::string testCompWhat (pch, pch+7);
                std::string testCompTo = "HTTP/1.";
                if (strcmp(&(testCompWhat[0]), &(testCompTo[0]))){
                    string beginPath = "/Users/pkyt/Desktop/github/WebServer/WebServerFirst/";
                    string restPath = pch;
                    string fileName;
                    pch = strtok(&(restPath[0]), "/");
                    while (pch != NULL){
                        fileName = pch;
                        pch = strtok(NULL, "/");
                    }
                    path = beginPath + restPath;
                    stat(&(path[0]), &fileStatus);
                    if (fileStatus.st_size == 0){ // file doesn't exist, ssince size o file  == 0
                        director.fileSend = error404;
                    }else{ // file exists
                        director.fileSend = attachment;
                        director.fileName = fileName;
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
                            contentType = "text/txt;";
                        }
                        director.contentType = contentType;
                        fileExists = true;
                    }
                }
            }else{
                // new code here
                
            }
        }
        director.construct(&wts);
        string whatToSend = wts.getWhatToSend();
        if (send(sock, &(whatToSend[0]), whatToSend.length(), 0))
            perror("send");
        if (fileExists){
            sendFile(sock, path, fileStatus.st_size);
        }
        // close(sock);
        cout << "message sent" << endl; */
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

