#include <signal.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include "port.h"
#include "service_helper.h"
#include "util.h"

namespace ServiceHelper {
static int serviceHelperFd = -1;
static std::string configDir;
static std::vector<std::pair<IpAddress, std::string>> startupData;

std::vector<std::pair<IpAddress, std::string>> getHostsFileAtStartup() {
  // used for only for migration
  return startupData;
}

bool modifyHostnameInternal(std::string newHostname) {
  if (getenv("HUSARNET_HOSTS_FILE"))
    return false;

  std::ifstream inp("/etc/hostname");
  std::string currentHostname;
  inp >> currentHostname;
  if (!inp.good())
    return false;
  if (currentHostname == newHostname) {
    return true;
  }
  if (!validateHostname(newHostname)) {
    LOG("invalid hostname ('%s')", newHostname.c_str());
    return false;
  }

  // various tools are sad when hostname doesn't point to any IPv4 address
  // TODO: modifyHostsInternal(HostAction::ADD, "127.0.0.1 " + newHostname);

  {
    std::ofstream out("/etc/hostname");
    out << newHostname << std::endl;
    if (!out.good()) {
      LOG("cannot overwrite /etc/hostname");
      return false;
    }
  }

  if (system("hostname -F /etc/hostname") != 0) {
    LOG("cannot update hostname");
    return false;
  }

  // TODO: modifyHostsInternal(HostAction::REMOVE, "127.0.0.1 " +
  // currentHostname);
  LOG("hostname changed to %s", newHostname.c_str());
  return true;
}

std::vector<std::pair<IpAddress, std::string>> deserializeHosts(
    std::string payload) {
  auto s = splitWhitespace(payload);
  std::vector<std::pair<IpAddress, std::string>> res;
  for (int i = 0; i + 1 < s.size(); i += 2) {
    res.push_back({IpAddress::parse(s[i]), s[i + 1]});
  }
  return res;
}

std::string serializeHosts(
    std::vector<std::pair<IpAddress, std::string>> payload) {
  std::string res;
  for (auto p : payload)
    if (validateHostname(p.second))
      res += p.first.str() + " " + p.second + " ";
  return res;
}

bool serviceHelperProc(int fd) {
  // so parent can't ptrace us
  if (prctl(PR_SET_DUMPABLE, 0) < 0) {
    perror("PR_SET_DUMPABLE");
    exit(1);
  }
  prctl(PR_SET_PDEATHSIG, SIGKILL);

  close(serviceHelperFd);
  char buf[40000];

  while (true) {
    long s = recv(fd, buf, sizeof(buf), 0);
    assert(s >= 1);

    std::string payload(buf + 1, s - 1);
    bool ok;
    if (buf[0] == 3) {
      ok = modifyHostnameInternal(payload);
    } else if (buf[0] == 2) {
      updateHostsFileInternal(deserializeHosts(payload));
    }
    if (send(fd, &ok, 1, 0) < 0) {
      perror("send");
      exit(1);
    }
  }
}

void startServiceHelperProc(std::string configDir) {
  startupData = getCurrentHostsFileInternal();

  // The code that modifies /etc/hosts cannot drop privileges.
  // We fork retaining full root and accept only certain requests from the low
  // privileged portion of code.
  int fds[2];
  if (socketpair(AF_UNIX, SOCK_DGRAM, 0, fds) < 0) {
    perror("socketpair");
    exit(1);
  }
  serviceHelperFd = fds[0];

  if (fork() == 0) {
    close(fds[0]);
    ServiceHelper::configDir = configDir;
    serviceHelperProc(fds[1]);
    _exit(1);
  } else {
    close(fds[1]);
  }
}

bool modifyHostname(std::string hostname) {
  if (serviceHelperFd == -1) {
    LOG("hostname modification disabled");
    return false;
  }

  std::string packet = "\3" + hostname;
  if (send(serviceHelperFd, packet.data(), packet.size(), 0) < 0) {
    perror("send");
    exit(1);
  }
  bool ok;
  if (recv(serviceHelperFd, &ok, 1, 0) < 0) {
    perror("recv");
    exit(1);
  }
  return ok;
}

bool updateHostsFile(std::vector<std::pair<IpAddress, std::string>> data) {
  if (serviceHelperFd == -1) {
    LOG("/etc/hosts modification disabled");
    return false;
  }
  std::string packet = ((char)'\2') + serializeHosts(data);
  if (send(serviceHelperFd, packet.data(), packet.size(), 0) < 0) {
    perror("send");
    exit(1);
  }
  bool ok;
  if (recv(serviceHelperFd, &ok, 1, 0) < 0) {
    perror("recv");
    exit(1);
  }
  return ok;
}
}  // namespace ServiceHelper
