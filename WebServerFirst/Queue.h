//
//  Queue.h
//  WebServerFirst
//
//  Created by Pavlo Kytsmey on 1/20/14.
//  Copyright (c) 2014 Pavlo Kytsmey. All rights reserved.
//

#ifndef WebServerFirst_Queue_h
#define WebServerFirst_Queue_h

#include <list>
#include <pthread.h>
#include <vector>

struct Task {
    int sock; // socket where to send
    std::vector<char> recvMsg; // received message
    Task(int s, std::vector<char> msg);
};

class Queue {
    std::list <Task> _tasks; // task list
    pthread_cond_t _cv;
    pthread_mutex_t _lock;
public:
    Queue();
    ~Queue();
    struct Task pop();
    void push(struct Task t);
};

#endif
