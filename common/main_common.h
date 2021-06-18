#pragma once
#include <string>
#include "util.h"
#include "port.h"

// implemented by platform code
std::string sendMsg(std::string msg);
void sendMsgChecked(std::string msg);
std::string getMyId();

// common functions
std::string getWebsetupData();
