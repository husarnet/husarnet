#include "service_helper.h"
#include "port.h"
#include "util.h"
#include <fstream>
#include "threads_port.h"

namespace ServiceHelper {
bool modifyHostname(std::string hostname) {
    return false;
}

std::vector<std::pair<IpAddress, std::string>> getHostsFileAtStartup() {
    // used for only for migration
    return getCurrentHostsFileInternal();
}

std::mutex updateLock;
std::condition_variable updateCv;
std::vector<std::pair<IpAddress, std::string>> updateData;

void updaterThread() {
    while (true) {
        std::vector<std::pair<IpAddress, std::string>> data;
        {
            std::unique_lock<std::mutex> guard {updateLock};
            updateCv.wait(guard);
            if (data == updateData) continue;
            data = updateData;
        }
        while (true) {
            LOG("updating hosts file...");
            bool ok = updateHostsFileInternal(data);
            if (ok) {
                LOG("updating hosts file OK");
                break;
            }
            LOG("hosts file update failed, retry in 2 seconds");
            sleep(2);
        }
    }
}

bool updateHostsFile(std::vector<std::pair<IpAddress, std::string>> data) {
    std::lock_guard<std::mutex> guard {updateLock};
    if (updateData != data) {
        updateData = data;
        updateCv.notify_all();
    }
    return true;
}

void startServiceHelperProc(std::string configDir) {
    startThread(updaterThread, "hosts-update");
}
}
