// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/husarnet_manager.h"

#include <map>
#include <vector>

#include "husarnet/ports/port.h"

#include "husarnet/compression_layer.h"
#include "husarnet/eventbus.h"
#include "husarnet/husarnet_config.h"
#include "husarnet/ipaddress.h"
#include "husarnet/layer_interfaces.h"
#include "husarnet/licensing.h"
#include "husarnet/logging.h"
#include "husarnet/multicast_layer.h"
#include "husarnet/peer_flags.h"
#include "husarnet/security_layer.h"
#include "husarnet/util.h"

#ifdef HTTP_CONTROL_API
#include "husarnet/api_server/server.h"
#endif

HusarnetManager::HusarnetManager()
{
  Port::init();
}

void HusarnetManager::prepareHusarnet()
{
  this->configEnv = new ConfigEnv();

  // Initialize logging and print some essential debugging information
  globalLogLevel = this->configEnv->getLogVerbosity();

  LOG_INFO("Running %s", HUSARNET_USER_AGENT);
  LOG_DEBUG("Running a nightly/debugging build");  // This macro has all the
                                                   // logic for not printing
                                                   // if not a debug build

  this->hooksManager = new HooksManager(this->configEnv->getEnableHooks());
  Port::threadStart(
      [this]() { this->hooksManager->periodicThread(); }, "hooks");

  this->configManager = new ConfigManager(this->hooksManager, this->configEnv);
  Port::threadStart(
      [this]() { this->configManager->periodicThread(); }, "config");
  this->configManager->waitInit();

  // At this point we have working settings/configuration and logs

  // Identity is not handled through the config so we need to initialize it
  // separately
  this->myIdentity = Identity::init();

  if (this->configManager->isInfraAddress(this->myIdentity->getIpAddress())) {
    LOG_INFO("HusarnetManager: considering self a part of control plane");
    this->configManager->setAllowEveryone();
  }

  // Make our PeerFlags available early if the caller wants to change them
  this->myFlags = new PeerFlags();
}

void HusarnetManager::runHusarnet()
{
  // Initialize the networking stack (peer container, tun/tap and various
  // ngsocket layers)
  this->peerContainer =
      new PeerContainer(this->configManager, this->myIdentity);

  auto tunTap = Port::startTunTap(
      this->myIdentity->getIpAddress(), this->configEnv->getDaemonInterface());
  this->tunTap = static_cast<TunTap*>(tunTap);

  auto multicast =
      new MulticastLayer(this->myIdentity->getDeviceId(), this->configManager);
  auto compression = new CompressionLayer(this->peerContainer, this->myFlags);
  this->securityLayer =
      new SecurityLayer(this->myIdentity, this->myFlags, this->peerContainer);
  this->ngsocket =
      new NgSocket(this->myIdentity, this->peerContainer, this->configManager);

  stackUpperOnLower(tunTap, multicast);
  stackUpperOnLower(multicast, compression);
  stackUpperOnLower(compression, securityLayer);
  stackUpperOnLower(securityLayer, ngsocket);

  Port::threadStart(
      [this]() {
        auto eventBus =
            new EventBus(this->myIdentity->getIpAddress(), this->configManager);
        eventBus->init();

        while(true) {
          eventBus->periodic();
        }
      },
      "eb");

// In case of a "fat" platform - start the API server
#ifdef HTTP_CONTROL_API
  auto proxy = new DashboardApiProxy(this->myIdentity, this->configManager);
  auto server = new ApiServer(this->configEnv, this->configManager, proxy);

  Port::threadStart([server]() { server->runThread(); }, "daemon_api");

  server->waitStarted();
#endif

  // Now we're pretty much good to go
  Port::notifyReady();

  // This is an actual event loop
  while(true) {
    ngsocket->periodic();

    Port::processSocketEvents(this->tunTap);
  }
}
