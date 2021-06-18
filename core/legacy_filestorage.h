#pragma once
#include "husarnet.h"
#include <unordered_set>
#include "configtable.h"

namespace LegacyFileStorage {
void migrateToConfigTable(ConfigTable* configTable, std::string configDir);
}
