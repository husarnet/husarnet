// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/husarnet_manager.h"

#include <map>
#include <vector>

#include <response.h>

#include "husarnet/ports/port.h"

#include "husarnet/compression_layer.h"
#include "husarnet/dashboardapi/response.h"
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
#include <magic_enum/magic_enum.hpp>

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
  Port::threadStart([this]() { this->hooksManager->periodicThread(); }, "hooks");

  // Identity is not handled through the config so we need to initialize it
  // separately
  this->myIdentity = Identity::init();

  // Initialize ConfigManager which will read what's on the disk and try fetching license and config
  this->configManager = new ConfigManager(this->hooksManager, this->configEnv, this->myIdentity->getIpAddress());
  Port::threadStart([this]() { this->configManager->periodicThread(); }, "config");
  this->configManager->waitInit();  // blocks until we know where (and if) to connect

  // At this point we have working settings/configuration and logs

  // Spin off another thread for heartbeats - report being alive to Dashboard every now and then
  if(this->configEnv->getEnableControlplane()) {
    Port::threadStart(
        [this]() {
          while(true) {
            const auto apiAddress = this->configManager->getApiAddress();
            if(apiAddress.isValid()) {
              dashboardapi::postHeartbeat(apiAddress, this->myIdentity);
            }
            Port::threadSleep(heartbeatPeriodMs);
          }
        },
        "heartbeat");
  }

  // Make our PeerFlags available early if the caller wants to change them
  this->myFlags = new PeerFlags();
}

void HusarnetManager::runHusarnet()
{
  // Initialize the networking stack (peer container, tun/tap and various
  // ngsocket layers)
  this->peerContainer = new PeerContainer(this->configManager, this->myIdentity);

  auto tt = Port::startTunTap(this->myIdentity->getIpAddress(), this->configEnv->getDaemonInterface());
  this->tunTap = dynamic_cast<TunTap*>(tt);

  auto multicast = new MulticastLayer(this->myIdentity->getDeviceId(), this->configManager);
  auto compression = new CompressionLayer(this->peerContainer, this->myFlags);
  this->securityLayer = new SecurityLayer(this->myIdentity, this->myFlags, this->peerContainer);
  this->ngsocket = new NgSocket(this->myIdentity, this->peerContainer, this->configManager);
  this->eventBus = new EventBus(this->myIdentity->getIpAddress(), this->configManager);

  stackUpperOnLower(tunTap, multicast);
  stackUpperOnLower(multicast, compression);
  stackUpperOnLower(compression, securityLayer);
  stackUpperOnLower(securityLayer, ngsocket);

  if(this->configEnv->getEnableControlplane()) {
    this->eventBus->init();
    Port::threadStart(
        [this]() {
          while(true) {
            this->eventBus->periodic();
            Port::threadSleep(500);
          }
        },
        "eb");
  }

// In case of a "fat" platform - start the API server
#ifdef HTTP_CONTROL_API
  auto server = new ApiServer(this->configEnv, this->configManager, this, this->myIdentity);

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

json HusarnetManager::getDataForStatus() const
{
  LOG_INFO("HusarnetManager: getDataForStatus");
  json result;

  result[STATUS_KEY_LOCALIP] = this->myIdentity->getIpAddress().toString();

  auto baseConnectionType = this->ngsocket->getCurrentBaseConnectionType();
  auto currentBaseAddress = this->ngsocket->getCurrentBaseAddress();

  result[STATUS_KEY_BASECONNECTION] = json::object({
      {STATUS_KEY_BASECONNECTION_TYPE, magic_enum::enum_name(baseConnectionType)},
      {STATUS_KEY_BASECONNECTION_ADDRESS, currentBaseAddress.ip.toString()},
      {STATUS_KEY_BASECONNECTION_PORT, currentBaseAddress.port},
  });

  bool isEventBusConnected = this->eventBus->isConnected();
  result[STATUS_KEY_DASHBOARDCONNECTION] = this->eventBus->isConnected();

  bool healthSummary = (baseConnectionType != BaseConnectionType::None) && isEventBusConnected;
  result[STATUS_KEY_HEALTH] = json::object({{STATUS_KEY_HEALTH_SUMMARY, healthSummary}});

  // add peers from peerContainer
  result[STATUS_KEY_LIVEPEERS] = json::array();
  auto peers = this->peerContainer->getPeers();
  for(auto& [peerId, rawPeer] : peers) {
    json newPeer = {
        {"address", rawPeer->getIpAddress().toString()},
        {"is_active", rawPeer->isActive()},
        {"is_reestablishing", rawPeer->isReestablishing()},
        {"is_tunelled", rawPeer->isTunelled()},
        {"is_secure", rawPeer->isSecure()},
    };

    result[STATUS_KEY_LIVEPEERS].push_back(newPeer);
  }
  return result;
}
