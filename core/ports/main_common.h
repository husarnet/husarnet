#pragma once
#include <string>
#include "port.h"
#include "util.h"

// implemented by platform code
std::string sendMsg(std::string msg);
void sendMsgChecked(std::string msg);
std::string getMyId();

// common functions
std::string getWebsetupData();
