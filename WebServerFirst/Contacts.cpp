//
//  Contacts.cpp
//  WebServerFirst
//
//  Created by Pavlo Kytsmey on 3/7/14.
//  Copyright (c) 2014 Pavlo Kytsmey. All rights reserved.
//

#include "Contacts.h"
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

int Contacts::getSocketID(std::string nickName){
    pthread_mutex_lock(&_lock);
    for (int i = 0; i < _contacts.size(); i++) {
        Contact curr = _contacts[i];
        if (!std::strcmp(&(curr.nickName[0]), &(nickName[0]))) {
            pthread_mutex_unlock(&_lock);
            return curr.socketID;
        }
    }
    pthread_mutex_unlock(&_lock);
    return -1;
}

std::vector<std::string> Contacts::getAllUsers(){
    pthread_mutex_lock(&_lock);
    std::vector<std::string> result;
    for (int i = 0; i < _contacts.size(); i++) {
        result.push_back(_contacts[i].nickName);
    }
    pthread_mutex_unlock(&_lock);
    return result;
}

std::string Contacts::pushContact(int socketID, std::string nickName){
    if(getSocketID(nickName) == -1){
        pthread_mutex_lock(&_lock);
        Contact newContact (socketID, nickName);
        _contacts.push_back(newContact);
        pthread_mutex_unlock(&_lock);
        return "success";
    }else{
        return "iam:failed";
    }
}

std::string Contacts::change(int socket, std::string nickName){
    pthread_mutex_lock(&_lock);
    for (int i = 0; i < _contacts.size(); i++) {
        Contact curr = _contacts[i];
        if (!std::strcmp(&(curr.nickName[0]), &(nickName[0]))) {
            close(curr.socketID);
            curr.socketID = socket;
            pthread_mutex_unlock(&_lock);
            return "success";
        }
    }
    pthread_mutex_unlock(&_lock);
    return "was:failed";
}

