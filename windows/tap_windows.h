#include "port.h"
#include "util.h"
#include "husarnet.h"
#include <mutex>

class WinTap {
    void* tap_fd;
    std::string deviceName;
public:
    static WinTap* create(std::string savedDeviceName);
    std::string getDeviceName() { return deviceName; }
    std::string getNetshName();
    void bringUp();
    std::string getMac();
    string_view read(std::string& buffer);
    void write(string_view data);
};
