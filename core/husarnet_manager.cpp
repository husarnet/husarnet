// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/husarnet_manager.h"

#include <map>
#include <vector>

#include "husarnet/ports/port.h"
#include "husarnet/ports/port_interface.h"
#include "husarnet/ports/privileged_interface.h"
#include "husarnet/ports/sockets.h"

#include "husarnet/compression_layer.h"
#include "husarnet/config_storage.h"
#include "husarnet/device_id.h"
#include "husarnet/husarnet_config.h"
#include "husarnet/ipaddress.h"
#include "husarnet/layer_interfaces.h"
#ifdef ENABLE_LEGACY_CONFIG
#include "husarnet/legacy_config.h"
#endif
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
  return Privileged::getSelfHostname();
}

void HusarnetManager::setConfigStorage(ConfigStorage* cs)
{
  this->configStorage = cs;
}

bool HusarnetManager::setSelfHostname(std::string newHostname)
{
  bool result = false;
  this->getHooksManager()->withRw(
      [&]() { result = Privileged::setSelfHostname(newHostname); });
  return result;
}

void HusarnetManager::updateHosts()
{
  if(this->dummyMode) {
    return;
  }

  this->getHooksManager()->withRw(
      [&]() { Privileged::updateHostsFile(configStorage->getHostTable()); });
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

int HusarnetManager::getApiPort()
{
  return configStorage->getUserSettingInt(UserSetting::daemonApiPort);
}

IpAddress HusarnetManager::getApiAddress()
{
  return configStorage->getUserSettingIp(UserSetting::daemonApiAddress);
}

std::string HusarnetManager::getApiInterface()
{
  return configStorage->getUserSetting(UserSetting::daemonApiInterface);
}

IpAddress HusarnetManager::getApiInterfaceAddress()
{
  return Port::getIpAddressFromInterfaceName(this->getApiInterface());
}

int HusarnetManager::getLogVerbosity()
{
  return configStorage->getUserSettingInt(UserSetting::logVerbosity);
}

void HusarnetManager::setLogVerbosity(int logLevel)
{
  getGlobalLogManager()->setVerbosity(logLevelFromInt(logLevel));

  configStorage->setUserSetting(UserSetting::logVerbosity, logLevel);
}

std::string HusarnetManager::getApiSecret()
{
  return Privileged::readApiSecret();
}

std::string HusarnetManager::rotateApiSecret()
{
  this->getHooksManager()->withRw([&]() { Privileged::rotateApiSecret(); });
  return this->getApiSecret();
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
  bool werentEnabled = !areHooksEnabled();

  configStorage->setUserSetting(UserSetting::enableHooks, trueValue);

  if(werentEnabled) {
    hooksManager = new HooksManager(this);
  }
}

void HusarnetManager::hooksDisable()
{
  configStorage->setUserSetting(UserSetting::enableHooks, falseValue);
}

bool HusarnetManager::areNotificationsEnabled()
{
  return configStorage->getUserSettingBool(UserSetting::enableNotifications);
}

void HusarnetManager::notificationsEnable()
{
  configStorage->setUserSetting(UserSetting::enableNotifications, trueValue);
}

void HusarnetManager::notificationsDisable()
{
  configStorage->setUserSetting(UserSetting::enableNotifications, falseValue);
}

std::list<std::string> HusarnetManager::getNotifications()
{
  if(notificationManager == nullptr) {
    return {};
  }
  return notificationManager->getNotifications();
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
  Privileged::init();

  this->hooksManager = new DummyHooksManager();  // This will be no-op at least
                                                 // until we can read settings
  
  this->notificationManager = new DummyNotificationManager(); // No-op until the
                                                              // join operation
}

HusarnetManager::~HusarnetManager()
{
  delete this->hooksManager;
}

void HusarnetManager::enterDummyMode()
{
  this->dummyMode = true;
}

void HusarnetManager::readLegacyConfig()
{
#ifdef ENABLE_LEGACY_CONFIG
  const std::string legacyConfigPath = Privileged::getLegacyConfigPath();
  if(!Port::isFile(legacyConfigPath)) {
    return;
  }

  LegacyConfig legacyConfig(legacyConfigPath);
  if(!legacyConfig.open()) {
    LOG_WARNING(
        "Legacy config is present, but couldn't read its contents on path: %s",
        legacyConfigPath.c_str());
    return;
  }

  LOG_WARNING(
      "Found legacy config on path: %s, will attempt to transfer the values to "
      "new "
      "format",
      legacyConfigPath.c_str());

  auto websetupSecretOld = legacyConfig.getWebsetupSecret();
  auto whitelistEnabledOld = legacyConfig.getWhitelistEnabled();
  auto whitelistOld = legacyConfig.getWhitelistEntries();

  configStorage->groupChanges([&]() {
    setWebsetupSecret(websetupSecretOld);
    for(auto& entry : whitelistOld) {
      whitelistAdd(IpAddress::parse(entry));
    }
    if(whitelistEnabledOld) {
      whitelistEnable();
    } else {
      whitelistDisable();
    }
  });

  this->getHooksManager()->withRw(
      [&]() { Port::renameFile(legacyConfigPath, legacyConfigPath + ".old"); });
#endif
}

void HusarnetManager::getLicenseStage()
{
  this->getHooksManager()->withRw([&]() {
    license =
        new License(configStorage->getUserSetting(UserSetting::dashboardFqdn));
  });
}

void HusarnetManager::getIdentityStage()
{
  // TODO long term - reenable the smartcard support but with proper
  // multiplatform support
  if(Privileged::checkValidIdentityExists()) {
    identity = Privileged::readIdentity();
  } else {
    this->getHooksManager()->withRw(
        [&]() { identity = Privileged::createIdentity(); });
  }
}

void HusarnetManager::startNetworkingStack()
{
  peerContainer = new PeerContainer(this);

  auto tunTap = Port::startTunTap(this);
  this->tunTap = static_cast<TunTap*>(tunTap); //TODO: cleanup

  Privileged::dropCapabilities();

  auto multicast = new MulticastLayer(this);
  auto compression = new CompressionLayer(this);
  securityLayer = new SecurityLayer(this);
  ngsocket = new NgSocket(this);

  stackUpperOnLower(tunTap, multicast);
  stackUpperOnLower(multicast, compression);
  stackUpperOnLower(compression, securityLayer);
  stackUpperOnLower(securityLayer, ngsocket);
}

void HusarnetManager::startWebsetup()
{
  websetup = new WebsetupConnection(this);
  websetup->start();
}

void HusarnetManager::startHTTPServer()
{
#ifdef HTTP_CONTROL_API
  threadpool.push_back(new std::thread([this]() {
    auto server = new ApiServer(this);
    server->runThread();
  }));
#endif
}

void HusarnetManager::stage1()
{
  if(stage1Started) {
    return;
  }

  Privileged::createConfigDirectories();
  Privileged::start();

  // At this point we have a working privileged interface

  configStorage = new ConfigStorage(
      this, Privileged::readConfig, Privileged::writeConfig, userDefaults,
      Port::getEnvironmentOverrides(), internalDefaults);

  readLegacyConfig();

  // This checks whether the dashboard URL was recently changed using an
  // environment variable and makes sure it's saved to a persistent storage (as
  // other values like websetup secret) depend on it.
  // This may be used i.e. in Docker container setup.
  if(configStorage->isUserSettingOverriden(UserSetting::dashboardFqdn)) {
    configStorage->persistUserSettingOverride(UserSetting::dashboardFqdn);
  }

  getGlobalLogManager()->setVerbosity(logLevelFromInt(this->getLogVerbosity()));
  LOG_INFO("Running %s", getUserAgent().c_str());
  LOG_DEBUG(
      "Running a nightly/debugging build");  // This macro has all the logic for
                                             // detecting build type internally
  configStorage->printSettings();

  // At this point we have working settings/configuration and logs

  selfFlags = new PeerFlags();

  stage1Started = true;
}

void HusarnetManager::stage2()
{
  if(stage2Started) {
    return;
  }

  getLicenseStage();
  getIdentityStage();

  stage2Started = true;
}

void HusarnetManager::stage3()
{
  if(stage3Started) {
    return;
  }

  if(configStorage->getUserSettingBool(UserSetting::enableHooks)) {
    this->hooksManager = new HooksManager(this);
  }

  startNetworkingStack();
  startWebsetup();

  if(configStorage->isUserSettingOverriden(UserSetting::joinCode)) {
    if(configStorage->isUserSettingOverriden(UserSetting::hostname)) {
      joinNetwork(
          configStorage->getUserSetting(UserSetting::joinCode),
          configStorage->getUserSetting(UserSetting::hostname));
    } else {
      joinNetwork(configStorage->getUserSetting(UserSetting::joinCode));
    }
  }

  this->hostTableAdd("husarnet-local", this->getSelfAddress());

  // Notifications are disabled on ESP32 platform
  #ifndef ESP_PLATFORM
  this->notificationManager = new NotificationManager(
      configStorage->getUserSetting(UserSetting::dashboardFqdn), this);
  #endif

  startHTTPServer();

  stage3Started = true;
}

void HusarnetManager::runHusarnet()
{
  stage1();
  stage2();
  stage3();

  Privileged::notifyReady();

  while(true) {
    ngsocket->periodic();
    Port::processSocketEvents(this);
  }
}
