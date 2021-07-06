#pragma once
#include <string>

class LogManager{
    class LogElement{
        public:
        std::string log;
        LogElement* next;
        LogElement* prev;
        LogElement(std::string log): log(log), next(nullptr), prev(nullptr) {};
    };
    uint size;
    uint currentSize;
    LogElement* first;
    LogElement* last;
    uint verbosity;
    public:
    LogManager(uint size): size(size), currentSize(0), first(nullptr), last(nullptr), verbosity(3) {};
    std::string getLogs();
    void setSize(uint size);
    void insert(std::string log);
    void setVerbosity(uint verb);
    uint16_t getVerbosity();
    
};