#pragma once
#include <unordered_set>
#include "configtable.h"
#include "husarnet.h"

namespace LegacyFileStorage {
void migrateToConfigTable(ConfigTable* configTable, std::string configDir);
}
