#pragma once
#include "configtable.h"

ConfigTable* createMemoryConfigTable(
    std::string initialData,
    std::function<void(std::string)> writeFunc);
