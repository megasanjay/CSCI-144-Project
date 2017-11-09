//
//  main.cpp
//  Project144
//
//  Created by Michelle Salomon on 11/2/17.
//  Copyright © 2017 Michelle Salomon. All rights reserved.
//

#include <iostream>
#include <mutex>
#include <condition_variable>
#include <pthread.h>
#include <time.h>
#include <sstream>
#include <string>
#include <unistd.h>
#include <assert.h>

using namespace std;

#define SIZE 1
#define THREAD_COUNT 4

mutex resource;
mutex cout_mutex;
mutex thread_mutex;
condition_variable cv;

int avail[SIZE] = {8};      // avail[i]: instances of resource i available
int maxAlloc[SIZE][THREAD_COUNT];   // max[i][j] max of resource i needed by thread j
int alloc[SIZE][THREAD_COUNT];      // alloc[i][j]: current allocation of resource i to thread j
static pthread_t threads[10];       // array to hold 10 threads
bool threadComplete(int tID);
int z = 0;

int checkIfAbleToFinish(int need[][THREAD_COUNT], bool finish[], int toBeAvail[])
{
    bool flag = false;
    
    for(int j = 0; j < THREAD_COUNT; j++)
    {
        if(finish[j] == false)
        {
            for(int i = 0; i < SIZE; i++)
            {
                if(need[i][j] > toBeAvail[i])
                {
                    flag = true;
                    break;
                }
            }
            
            if(flag)
            {
                flag = false;
            }
            else
            {
                return j;
            }
        }
    }
    
    return -1;
}

bool isSafe()
{
    int j;
    int toBeAvail[SIZE];
    int need[SIZE][THREAD_COUNT];
    bool finish[THREAD_COUNT];
    
    for(int i = 0; i < SIZE; i++)
    {
        toBeAvail[i] = avail[i];
    }
    
    for(int i = 0; i < SIZE; i++)
    {
        for(int j = 0; j < THREAD_COUNT; j++)
        {
            need[i][j] = maxAlloc[i][j] - alloc[i][j];
        }
    }
    
    for(int i = 0; i < THREAD_COUNT; i++)
    {
        finish[i] = false;
    }
    
    while(true)
    {
        j = checkIfAbleToFinish(need, finish, toBeAvail);
        
        if(j == -1)
        {
            for(int x = 0; x < THREAD_COUNT; x++)
            {
                if(finish[x] == false)
                {
                    return false;
                }
            }
            
            return true;
        }
        else
        {
            // thread j will eventually finish and return it's current allocation to the pool
            finish[j] = true;
            
            for(int i = 0; i < SIZE; i++)
            {
                toBeAvail[i] = toBeAvail[i] + alloc[i][j];
            }
        }
    }
}

bool wouldBeSafe(int resourceID, int threadID)
{
    bool result = false;
    
    avail[resourceID]--;
    alloc[resourceID][threadID]++;
    
    if (isSafe())
    {
        result = true;
    }
    
    avail[resourceID]++;
    alloc[resourceID][threadID]--;
    
    //ready = false;
    
    return result;
}

void request(int resourceID, int threadID)
{
    stringstream output;
    
    unique_lock<mutex> lk(resource);
    
    {
        unique_lock<mutex> ioLock(cout_mutex);
        output.str("");
        
        output << "\nThread " << to_string(threadID) << " is requesting resource " << to_string(resourceID) << ". Available amount of this resource is " << avail[resourceID];
        output << ". This thread needs " << (maxAlloc[resourceID][threadID] - alloc[resourceID][threadID]);
        output << " more of this resource at this moment...";
        
        cout << output.str() << endl;
        ioLock.unlock();
    }
    
    assert(isSafe());
    while (!wouldBeSafe(resourceID, threadID))
    {
        {
            unique_lock<mutex> ioLock(cout_mutex);
            output.str("");
            
            output << "Assignment of this request might result in an unsafe state for this program... Thread " << threadID << " going into waiting... Available resource for this ammount is " << avail[resourceID];
            
            cout << output.str() << endl;
            ioLock.unlock();
        }
        cv.wait(lk, []{return z == 1;});
        z = 0;
    }
    
    alloc[resourceID][threadID]++;
    avail[resourceID]--;
    
    {
        unique_lock<mutex> ioLock(cout_mutex);
        output.str("");
        output << endl << "Thread " << threadID << " has received resource " << resourceID << ".";
        
        cout << output.str() << endl;
        ioLock.unlock();
    }
    assert(isSafe());
    lk.unlock();
    
    return;
}

bool threadComplete(int tID)
{
    for (int i = 0; i < SIZE; i++)
    {
        if (alloc[i][tID] < maxAlloc[i][tID])
        {
            return false;
        }
    }
    
    return true;
}

void* threadFunction(void* i)
{
    int *j = (int*)i;
    int resourceID = 0;
    int threadID = *j;
    stringstream output;
    
    
    
    while(!threadComplete(threadID))
    {
        unique_lock<mutex> threadLock(thread_mutex);
        
        resourceID = rand() % SIZE;
        
        if (alloc[resourceID][threadID] < maxAlloc[resourceID][threadID])
        {
            request(resourceID, threadID);
        }
        
        //ready = true;
        z=1;
        threadLock.unlock();
        cv.notify_one();
        
        //usleep(1000);
        //sleep(1);
    }
    
    for(int i = 0; i < SIZE; i++)
    {
        avail[i] += alloc[i][threadID];
    }
    
    unique_lock<mutex> ioLock(cout_mutex);
    output.str("");
    output << "-------------------------\n";
    output << "Thread " << threadID << " is complete...\n";
    output << "-------------------------";
    cout << output.str() << endl;
    ioLock.unlock();

    
    pthread_exit(NULL);
}

void* threadie(void* y)
{
    for(int i = 0; i < THREAD_COUNT; i++)
    {
        int *x = new int;
        *x = i;
        pthread_create(&threads[i], NULL, threadFunction, x);
    }
    
    for(int i = 0; i < THREAD_COUNT; i++)
    {
        pthread_join(threads[i],NULL);
        //cout << endl << "JOIN!!!!pls..." << x << endl;
    }
    
    pthread_exit(NULL);
}

int main(int argc, const char * argv[])
{
    srand((int) time(NULL));
    
    pthread_t t1;
    
    for(int i = 0; i < THREAD_COUNT; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            alloc[j][i] = 0;
            maxAlloc[j][i] = rand() % 7;
        }
    }
    
    pthread_create(&t1, NULL, threadie, NULL);
    pthread_join(t1,NULL);
    
    return 0;
}
