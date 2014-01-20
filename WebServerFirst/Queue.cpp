//
//  Queue.cpp
//  WebServerFirst
//
//  Created by Pavlo Kytsmey on 1/20/14.
//  Copyright (c) 2014 Pavlo Kytsmey. All rights reserved.
//

#include "Queue.h"


Task::Task(int s, std::vector<char> msg){
    sock = s;
    recvMsg = msg;
}

Queue::Queue(){
    pthread_cond_init(&_cv, NULL);
    pthread_mutex_init(&_lock, NULL);
}

Queue::~Queue(){
    pthread_cond_destroy(&_cv);
    pthread_mutex_destroy(&_lock);
}

struct Task Queue::pop(){
    pthread_mutex_lock(&_lock); // starting working with shared variable
    while (_tasks.size() == 0){
        pthread_cond_wait(&_cv, &_lock); // waiting until new task appear
    }
    struct Task t = _tasks.front(); // getting the first task from _task list
    _tasks.pop_front(); // erasing from list (the given task to thread)
    pthread_mutex_unlock(&_lock);
    return t;
}

void Queue::push(struct Task t){
    pthread_mutex_lock(&_lock); // starting working with shared variable
    _tasks.push_back(t); // adding new Taks into _tasks list at the back
    pthread_cond_signal(&_cv); // signaling that new Task appeared
    pthread_mutex_unlock(&_lock);
}