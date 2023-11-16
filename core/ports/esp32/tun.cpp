// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ports/esp32/tun.h"

#include "husarnet/ports/port.h"

#include "husarnet/logging.h"
#include "husarnet/util.h"

TunTap::TunTap()
{
}

void TunTap::onLowerLayerData(DeviceId source, string_view data)
{
}
