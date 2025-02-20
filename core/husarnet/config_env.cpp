// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/config_env.h"

#include "husarnet/ports/port_interface.h"

#include "magic_enum/magic_enum.hpp"

static const std::string envPresentOrDefault(
    etl::map<EnvKey, std::string, ENV_KEY_OPTIONS> env,
    const EnvKey key,
    const std::string& def)
{
  if(env.contains(key)) {
    return env.at(key);
  } else {
    return def;
  }
}

ConfigEnv::ConfigEnv()
{
  this->env = Port::getEnvironmentOverrides();
}

json ConfigEnv::getJson() const
{
  json j;
  j["tldFqdn"] = getTldFqdn();
  j["logVerbosity"] = magic_enum::enum_name(getLogVerbosity());
  j["enableHooks"] = getEnableHooks();
  j["enableControlPlane"] = getEnableControlplane();
  j["daemonInterface"] = getDaemonInterface();
  j["daemonApiInterface"] = getDaemonApiInterface();
  j["daemonApiHost"] = getDaemonApiHost().toString();
  j["daemonApiPort"] = getDaemonApiPort();
  return j;
}

std::string ConfigEnv::getTldFqdn() const
{
  return envPresentOrDefault(this->env, EnvKey::tldFqdn, "prod.husarnet.com");
}

LogLevel ConfigEnv::getLogVerbosity() const
{
#if defined(ESP_PLATFORM)
  LogLevel def = CONFIG_HUSARNET_LOG_LEVEL;
#elif defined(DEBUG_BUILD)
  auto def = LogLevel::DEBUG;
#else
  auto def = LogLevel::INFO;
#endif

  if(this->env.contains(EnvKey::logVerbosity)) {
    auto requested = strToUpper(env.at(EnvKey::logVerbosity));
    return magic_enum::enum_cast<LogLevel>(requested).value_or(def);
  }

  return def;
}

bool ConfigEnv::getEnableHooks() const
{
  return strToBool(
      envPresentOrDefault(this->env, EnvKey::enableHooks, "false"));
}

bool ConfigEnv::getEnableControlplane() const
{
  return strToBool(
      envPresentOrDefault(this->env, EnvKey::enableControlPlane, "true"));
}

const std::string ConfigEnv::getDaemonInterface() const
{
  return envPresentOrDefault(this->env, EnvKey::daemonInterface, "hnet0");
}

const std::string ConfigEnv::getDaemonApiInterface() const
{
  return envPresentOrDefault(this->env, EnvKey::daemonApiInterface, "");
}

const InternetAddress ConfigEnv::getDaemonApiHost() const
{
  auto def = InternetAddress::parse("127.0.0.1");

  if(this->env.contains(EnvKey::daemonApiHost)) {
    return InternetAddress::parse(
        this->env.at(EnvKey::daemonApiHost));  // TODO maybe add a sanity check
                                               // and a nice user-facing message
  }

  return def;
}

int ConfigEnv::getDaemonApiPort() const
{
  return std::stoi(
      envPresentOrDefault(this->env, EnvKey::daemonApiPort, "16216"));
}
