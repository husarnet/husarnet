// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/husarnet_manager.h"

#include <map>
#include <vector>

#include "husarnet/ports/port.h"
#include "husarnet/ports/sockets.h"

#include "husarnet/compression_layer.h"
#include "husarnet/config_storage.h"
#include "husarnet/device_id.h"
#include "husarnet/husarnet_config.h"
#include "husarnet/ipaddress.h"
#include "husarnet/layer_interfaces.h"
#include "husarnet/licensing.h"
#include "husarnet/logging.h"
#include "husarnet/multicast_layer.h"
#include "husarnet/ngsocket.h"
#include "husarnet/peer_container.h"
#include "husarnet/peer_flags.h"
#include "husarnet/security_layer.h"
#include "husarnet/util.h"
#include "husarnet/websetup.h"

#ifdef HTTP_CONTROL_API
#include "husarnet/api_server/server.h"
#endif

ConfigStorage& HusarnetManager::getConfigStorage()
{
  return *configStorage;
}

PeerContainer* HusarnetManager::getPeerContainer()
{
  return peerContainer;
}

TunTap* HusarnetManager::getTunTap()
{
  return tunTap;
}

HooksManagerInterface* HusarnetManager::getHooksManager()
{
  return hooksManager;
}

std::string HusarnetManager::getVersion()
{
  return HUSARNET_VERSION;
}

std::string HusarnetManager::getUserAgent()
{
  return std::string("Husarnet,") + strToLower(PORT_NAME) + "," +
         strToLower(PORT_ARCH) + "," + HUSARNET_VERSION;
}

Identity* HusarnetManager::getIdentity()
{
  return &identity;
}

IpAddress HusarnetManager::getSelfAddress()
{
  return identity.getIpAddress();
}

PeerFlags* HusarnetManager::getSelfFlags()
{
  return selfFlags;
}

std::string HusarnetManager::getSelfHostname()
{
  return rtrim(Port::getSelfHostname());
}

void HusarnetManager::setConfigStorage(ConfigStorage* cs)
{
  this->configStorage = cs;
}

bool HusarnetManager::setSelfHostname(std::string newHostname)
{
  bool result = false;
  this->getHooksManager()->withRw(
      [&]() { result = Port::setSelfHostname(newHostname); });
  return result;
}

void HusarnetManager::updateHosts()
{
  if(this->dummyMode) {
    return;
  }

  this->getHooksManager()->withRw(
      [&]() { Port::updateHostsFile(configStorage->getHostTable()); });
}

IpAddress HusarnetManager::resolveHostname(std::string hostname)
{
  auto hostTable = configStorage->getHostTable();

  if(hostTable.find(hostname) == hostTable.end()) {
    return IpAddress{};
  }

  return hostTable[hostname];
}

InetAddress HusarnetManager::getCurrentBaseAddress()
{
  return ngsocket->getCurrentBaseAddress();
}

std::string HusarnetManager::getCurrentBaseProtocol()
{
  return ngsocket->getCurrentBaseConnectionType()._to_string();
}

bool HusarnetManager::isConnectedToBase()
{
  return ngsocket->getCurrentBaseConnectionType() != +BaseConnectionType::NONE;
}

bool HusarnetManager::isConnectedToWebsetup()
{
  auto websetupPeer =
      peerContainer->getPeer(deviceIdFromIpAddress(getWebsetupAddress()));
  if(websetupPeer == NULL) {
    return false;
  }

  return websetupPeer->isSecure();
}

bool HusarnetManager::isDirty()
{
  return dirty;
}

void HusarnetManager::setDirty()
{
  // Yes, this is only a runtime flag
  dirty = true;
}

std::string HusarnetManager::getWebsetupSecret()
{
  return configStorage->getInternalSetting(InternalSetting::websetupSecret);
}

std::string HusarnetManager::setWebsetupSecret(std::string newSecret)
{
  configStorage->setInternalSetting(InternalSetting::websetupSecret, newSecret);
  return newSecret;
}

static std::string parseJoinCode(std::string joinCode)
{
  int slash = joinCode.find("/");
  if(slash == -1) {
    return joinCode;
  }

  return joinCode.substr(slash + 1);
}

void HusarnetManager::joinNetwork(std::string joinCode, std::string newHostname)
{
  this->whitelistAdd(this->getWebsetupAddress());

  std::string dashboardHostname;
  if(newHostname.empty()) {
    dashboardHostname = this->getSelfHostname();
  } else {
    this->setSelfHostname(newHostname);
    dashboardHostname = newHostname;
  }

  websetup->join(parseJoinCode(joinCode), dashboardHostname);
}

bool HusarnetManager::isJoined()
{
  // TODO long-term - add a periodic latency check for websetup
  // auto websetupId = deviceIdFromIpAddress(getWebsetupAddress());
  // getLatency(websetupId) >= 0
  return configStorage->isOnWhitelist(getWebsetupAddress()) &&
         websetup->getLastInitReply() > 0;
}

bool HusarnetManager::changeServer(std::string domain)
{
  if(!License::validateDashboard(domain)) {
    LOG_ERROR(
        "%s does not contain a valid licensing endpoint.", domain.c_str());
    return false;
  }

  configStorage->setUserSetting(UserSetting::dashboardFqdn, domain);
  setDirty();
  LOG_WARNING("Dashboard URL has been changed to %s.", domain.c_str());
  LOG_WARNING("DAEMON WILL CONTINUE TO USE THE OLD ONE UNTIL YOU RESTART IT");

  return true;
}

void HusarnetManager::hostTableAdd(std::string hostname, IpAddress address)
{
  configStorage->hostTableAdd(hostname, address);
}

void HusarnetManager::hostTableRm(std::string hostname)
{
  configStorage->hostTableRm(hostname);
}

void HusarnetManager::whitelistAdd(IpAddress address)
{
  configStorage->whitelistAdd(address);
}

void HusarnetManager::whitelistRm(IpAddress address)
{
  configStorage->whitelistRm(address);
}

std::list<IpAddress> HusarnetManager::getWhitelist()
{
  return configStorage->getWhitelist();
}

bool HusarnetManager::isWhitelistEnabled()
{
  return configStorage->getUserSettingBool(UserSetting::enableWhitelist);
}

void HusarnetManager::whitelistEnable()
{
  configStorage->setUserSetting(UserSetting::enableWhitelist, trueValue);
}

void HusarnetManager::whitelistDisable()
{
  configStorage->setUserSetting(UserSetting::enableWhitelist, falseValue);
}

bool HusarnetManager::isPeerAddressAllowed(IpAddress address)
{
  if(!isWhitelistEnabled()) {
    return true;
  }

  return configStorage->isOnWhitelist(address);
}

bool HusarnetManager::isRealAddressAllowed(InetAddress addr)
{
  return true;
}

LogLevel HusarnetManager::getVerbosity()
{
  auto rawValue = configStorage->getUserSettingInt(UserSetting::logVerbosity);
  auto level = LogLevel::_from_integral_nothrow(rawValue);

  // Known and reasonable level
  if(level) {
    return level.value();
  }
  // Unknown and negative level
  if(rawValue < 0) {
    return LogLevel::NONE;
  }

  // Unknown and large integer (more than 5-ish :D)
  return LogLevel::DEBUG;
}

void HusarnetManager::setVerbosity(LogLevel level)
{
  configStorage->setUserSetting(
      UserSetting::logVerbosity, level._to_integral());

  globalLogLevel = level;
}

int HusarnetManager::getDaemonApiPort()
{
  return configStorage->getUserSettingInt(UserSetting::daemonApiPort);
}

IpAddress HusarnetManager::getDaemonApiAddress()
{
  return configStorage->getUserSettingIp(UserSetting::daemonApiAddress);
}

std::string HusarnetManager::getDaemonApiInterface()
{
  return configStorage->getUserSetting(UserSetting::daemonApiInterface);
}

IpAddress HusarnetManager::getDaemonApiInterfaceAddress()
{
  return Port::getIpAddressFromInterfaceName(this->getDaemonApiInterface());
}

std::string HusarnetManager::getDaemonApiSecret()
{
  auto secret = Port::readApiSecret();
  if(secret.empty()) {
    rotateDaemonApiSecret();
    secret = Port::readApiSecret();
  }

  return secret;
}

std::string HusarnetManager::rotateDaemonApiSecret()
{
  this->getHooksManager()->withRw(
      [&]() { Port::writeApiSecret(generateRandomString(32)); });
  return this->getDaemonApiSecret();
}

std::string HusarnetManager::getDashboardFqdn()
{
  return license->getDashboardFqdn();
}
IpAddress HusarnetManager::getWebsetupAddress()
{
  return license->getWebsetupAddress();
}
std::vector<IpAddress> HusarnetManager::getBaseServerAddresses()
{
  return license->getBaseServerAddresses();
}
std::vector<IpAddress> HusarnetManager::getDashboardApiAddresses()
{
  return license->getDashboardApiAddresses();
}
std::vector<IpAddress> HusarnetManager::getEbAddresses()
{
  return license->getEbAddresses();
}

NgSocket* HusarnetManager::getNGSocket()
{
  return ngsocket;
}

SecurityLayer* HusarnetManager::getSecurityLayer()
{
  return securityLayer;
}

std::string HusarnetManager::getInterfaceName()
{
  return configStorage->getUserSetting(UserSetting::interfaceName);
}

void HusarnetManager::setInterfaceName(std::string name)
{
  configStorage->setUserSetting(UserSetting::interfaceName, name);
  // TODO / ympek could return
  // now there is some inconsistency in how setters work
  // also this should probably be internal setting in windows case
}

bool HusarnetManager::areHooksEnabled()
{
  return configStorage->getUserSettingBool(UserSetting::enableHooks);
}

void HusarnetManager::hooksEnable()
{
  if(areHooksEnabled()) {
    return;
  }

  configStorage->setUserSetting(UserSetting::enableHooks, trueValue);
  hooksManager = new HooksManager(this);
}

void HusarnetManager::hooksDisable()
{
  configStorage->setUserSetting(UserSetting::enableHooks, falseValue);
}

std::vector<DeviceId> HusarnetManager::getMulticastDestinations(DeviceId id)
{
  if(!id == deviceIdFromIpAddress(multicastDestination)) {
    return {};
  }

  std::vector<DeviceId> r;
  for(auto& ipAddress : configStorage->getWhitelist()) {
    r.push_back(deviceIdFromIpAddress(ipAddress));
  }
  return r;
}

int HusarnetManager::getLatency(DeviceId destination)
{
  return securityLayer->getLatency(destination);
}

void HusarnetManager::cleanup()
{
  configStorage->groupChanges([&]() {
    configStorage->whitelistClear();
    configStorage->whitelistAdd(getWebsetupAddress());
    configStorage->hostTableClear();
    configStorage->hostTableAdd("husarnet-local", getSelfAddress());
  });
}

HusarnetManager::HusarnetManager()
{
  Port::init();

  this->hooksManager = new DummyHooksManager();  // This will be no-op at least
                                                 // until we can read settings
}

HusarnetManager::~HusarnetManager()
{
  delete this->hooksManager;
}

void HusarnetManager::enterDummyMode()
{
  this->dummyMode = true;
}

void HusarnetManager::prepareHusarnet()
{
  // Initialize config
  configStorage = new ConfigStorage(
      this, Port::readConfig, Port::writeConfig, userDefaults,
      Port::getEnvironmentOverrides(), internalDefaults);

  // Check whether hooks were enabled and initialize them if so
  if(configStorage->getUserSettingBool(UserSetting::enableHooks)) {
    this->hooksManager = new HooksManager(this);
  }

  // This checks whether the dashboard URL was recently changed using an
  // environment variable and makes sure it's saved to a persistent storage (as
  // other values like websetup secret) depend on it.
  // This may be used i.e. in Docker container setup.
  if(configStorage->isUserSettingOverriden(UserSetting::dashboardFqdn)) {
    configStorage->persistUserSettingOverride(UserSetting::dashboardFqdn);
  }

  // Initialize logging and print some essential debugging information
  globalLogLevel = getVerbosity();

  LOG_INFO("Running %s", getUserAgent().c_str());
  LOG_DEBUG("Running a nightly/debugging build");  // This macro has all the
                                                   // logic for not printing if
                                                   // not a debug build

  configStorage->printSettings();

  // At this point we have working settings/configuration and logs

  // Make our PeerFlags available early if the caller wants to change them
  selfFlags = new PeerFlags();
}

void HusarnetManager::runHusarnet()
{
  // Get the license (download or use cached)
  this->getHooksManager()->withRw([&]() {
    license =
        new License(configStorage->getUserSetting(UserSetting::dashboardFqdn));
  });

  // Prepare identity (create a new one if not available)
  auto identityStr = Port::readIdentity();
  bool invalidIdentity = false;

  if(identityStr.empty()) {
    invalidIdentity = true;
  }
  if(!invalidIdentity) {
    identity = Identity::deserialize(identityStr);
    if(!identity.isValid()) {
      invalidIdentity = true;
    }
  }

  if(invalidIdentity) {
    this->getHooksManager()->withRw([&]() {
      identity = Identity::create();
      Port::writeIdentity(identity.serialize());
    });
  }

  // Initialize the networking stack (peer container, tun/tap and various
  // ngsocket layers)
  peerContainer = new PeerContainer(this);

  auto tunTap = Port::startTunTap(this);
  this->tunTap = static_cast<TunTap*>(tunTap);

  auto multicast = new MulticastLayer(this);
  auto compression = new CompressionLayer(this);
  securityLayer = new SecurityLayer(this);
  ngsocket = new NgSocket(this);

  stackUpperOnLower(tunTap, multicast);
  stackUpperOnLower(multicast, compression);
  stackUpperOnLower(compression, securityLayer);
  stackUpperOnLower(securityLayer, ngsocket);

  // Initialize websetup
  websetup = new WebsetupConnection(this);
  websetup->start();

  whitelistAdd(getWebsetupAddress());

  // TODO move this to websetup/dashboard
  this->hostTableAdd("husarnet-local", this->getSelfAddress());

  // If user provided a joincode via environment variables - now's the point to
  // use it Note: this case is different from the dashboard URL one as we do not
  // save it to a persistent storage
  if(configStorage->isUserSettingOverriden(UserSetting::joinCode)) {
    if(configStorage->isUserSettingOverriden(UserSetting::hostname)) {
      joinNetwork(
          configStorage->getUserSetting(UserSetting::joinCode),
          configStorage->getUserSetting(UserSetting::hostname));
    } else {
      joinNetwork(configStorage->getUserSetting(UserSetting::joinCode));
    }
  }

// In case of a "fat" platform - start the API server
#ifdef HTTP_CONTROL_API
  auto proxy = new DashboardApiProxy(this);
  auto server = new ApiServer(this, proxy);

  threadpool.push_back(new std::thread([=]() { server->runThread(); }));
  server->waitStarted();
#endif

  // Now we're pretty much good to go
  Port::notifyReady();

  // This is an actual event loop
  while(true) {
    ngsocket->periodic();
    Port::processSocketEvents(this);
  }
}
