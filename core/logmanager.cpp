#include "logmanager.h"

std::string LogManager::getLogs(){
    if(currentSize == 0) return "No logs to display";
    LogElement* itr = first;
    std::string result = "";
    while(itr != nullptr){
        result += itr->log;
        result += "\n";
        itr = itr->next;
    }
    return result;
};

void LogManager::insert(std::string log){
    LogElement * logElement = new LogElement(log);
    if(last == nullptr or size == 1){
        if(first != nullptr) delete first;
        currentSize = 1;
        first = logElement;
        last = logElement;
        return;
    }

    if(currentSize<=size){
        currentSize++;
        first->prev = logElement;
        logElement->next = first;
        first = logElement;
        return;
    }

    logElement->next = first;
    first->prev = logElement;
    first = logElement;
    last->prev->next = nullptr;
    LogElement* tmp = last;
    last = last->prev;
    delete tmp;
};

 void LogManager::setSize(uint size){
    if(size >= currentSize){
        this->size = size;
        return;
    }
    if(size<1){
        return;
    }
    if(size == 1){
        LogElement* itr = first->next;
        first->next = nullptr;
        last = first;
        this->size = size;
        while(itr != nullptr){
            LogElement* tmp = itr;
            itr = itr->next;
            delete tmp; 
        }
    }

    LogElement* itr = first->next;
    LogElement* followItr = first;
    int visitedCount = 2;
    while(visitedCount<size && itr !=nullptr){
        followItr = itr;
        itr = itr->next;
        visitedCount++;
    }
    while(itr != nullptr){
        LogElement* tmp = itr;
        itr = itr->next;
        delete tmp;

    }
    last = followItr;
    last->next = nullptr;
    this->size = size;
    this->currentSize = size;

 };

   void LogManager::setVerbosity(uint verb){
       verbosity = verb;
   };

   uint16_t LogManager::getVerbosity(){
       return verbosity;
   };