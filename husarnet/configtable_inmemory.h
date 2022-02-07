#pragma once
#include "configtable_interface.h"

ConfigTable* createMemoryConfigTable(
    std::string initialData,
    std::function<void(std::string)> writeFunc);
