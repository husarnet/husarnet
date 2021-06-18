#pragma once
#include "husarnet.h"
#include "configmanager.h"

namespace FileStorage {

std::ifstream openFile(std::string name);
Identity* readIdentity(std::string configDir);
void generateAndWriteId(std::string configDir);

}
