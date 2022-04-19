// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "tap_windows.h"
#include "gil.h"
#include "husarnet.h"
#include "port.h"
#include "util.h"
// Based on code from libtuntap (https://github.com/LaKabane/libtuntap, ISC
// License)

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383
#define NETWORK_ADAPTERS                                                 \
  "SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-" \
  "08002BE10318}"

/* From OpenVPN tap driver, common.h */
#define TAP_CONTROL_CODE(request, method) \
  CTL_CODE(FILE_DEVICE_UNKNOWN, request, method, FILE_ANY_ACCESS)
#define TAP_IOCTL_GET_MAC TAP_CONTROL_CODE(1, METHOD_BUFFERED)
#define TAP_IOCTL_GET_VERSION TAP_CONTROL_CODE(2, METHOD_BUFFERED)
#define TAP_IOCTL_GET_MTU TAP_CONTROL_CODE(3, METHOD_BUFFERED)
#define TAP_IOCTL_GET_INFO TAP_CONTROL_CODE(4, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_POINT_TO_POINT TAP_CONTROL_CODE(5, METHOD_BUFFERED)
#define TAP_IOCTL_SET_MEDIA_STATUS TAP_CONTROL_CODE(6, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_DHCP_MASQ TAP_CONTROL_CODE(7, METHOD_BUFFERED)
#define TAP_IOCTL_GET_LOG_LINE TAP_CONTROL_CODE(8, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_DHCP_SET_OPT TAP_CONTROL_CODE(9, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_TUN TAP_CONTROL_CODE(10, METHOD_BUFFERED)

typedef unsigned long IPADDR;

static std::vector<std::string> getExistingDeviceNames() {
  std::vector<std::string> names;
  const char* key_name = NETWORK_ADAPTERS;
  HKEY adapters, adapter;
  DWORD i, ret, len;
  DWORD sub_keys = 0;

  ret =
      RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT(key_name), 0, KEY_READ, &adapters);
  if (ret != ERROR_SUCCESS) {
    LOG("RegOpenKeyEx returned error");
    return {};
  }

  ret = RegQueryInfoKey(adapters, NULL, NULL, NULL, &sub_keys, NULL, NULL, NULL,
                        NULL, NULL, NULL, NULL);
  if (ret != ERROR_SUCCESS) {
    LOG("RegQueryInfoKey returned error");
    return {};
  }

  if (sub_keys <= 0) {
    LOG("Wrong registry key");
    return {};
  }

  /* Walk througt all adapters */
  for (i = 0; i < sub_keys; i++) {
    char new_key[MAX_KEY_LENGTH];
    char data[256];
    TCHAR key[MAX_KEY_LENGTH];
    DWORD keylen = MAX_KEY_LENGTH;

    /* Get the adapter key name */
    ret = RegEnumKeyEx(adapters, i, key, &keylen, NULL, NULL, NULL, NULL);
    if (ret != ERROR_SUCCESS) {
      continue;
    }

    /* Append it to NETWORK_ADAPTERS and open it */
    snprintf(new_key, sizeof new_key, "%s\\%s", key_name, key);
    ret =
        RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT(new_key), 0, KEY_READ, &adapter);
    if (ret != ERROR_SUCCESS) {
      continue;
    }

    /* Check its values */
    len = sizeof data;
    ret =
        RegQueryValueEx(adapter, "ComponentId", NULL, NULL, (LPBYTE)data, &len);
    if (ret != ERROR_SUCCESS) {
      /* This value doesn't exist in this adaptater tree */
      goto clean;
    }
    /* If its a tap adapter, its all good */
    if (strncmp(data, "tap", 3) == 0) {
      DWORD type;

      len = sizeof data;
      ret = RegQueryValueEx(adapter, "NetCfgInstanceId", NULL, &type,
                            (LPBYTE)data, &len);
      if (ret != ERROR_SUCCESS) {
        LOG("RegQueryValueEx returned error");
        goto clean;
      }
      LOG("found tap device: %s", data);

      names.push_back(std::string(data));
      // break;
    }
  clean:
    RegCloseKey(adapter);
  }
  RegCloseKey(adapters);
  return names;
}

static std::string getNetshNameForGuid(std::string guid) {
  std::string path =
      "SYSTEM\\CurrentControlSet\\Control\\Network\\{4d36e972-e325-11ce-bfc1-"
      "08002be10318}\\" +
      guid + "\\connection";

  DWORD dwType = REG_SZ;
  HKEY hKey = 0;
  char value[1024];
  DWORD value_length = 1024;
  RegOpenKey(HKEY_LOCAL_MACHINE, path.c_str(), &hKey);
  RegQueryValueEx(hKey, "name", NULL, &dwType, (LPBYTE)&value, &value_length);

  return value;
}

static HANDLE tuntapStart(std::string name) {
  HANDLE tun_fd;

  std::string path = "\\\\.\\Global\\" + name + ".tap";
  tun_fd = CreateFile(path.c_str(), GENERIC_WRITE | GENERIC_READ,
                      FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING,
                      FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED, 0);
  if (tun_fd == INVALID_HANDLE_VALUE) {
    int errcode = GetLastError();

    LOG("failed to open tap device: %d", errcode);
    return INVALID_HANDLE_VALUE;
  }

  return tun_fd;
}

static void createNewTuntap() {
  system("addtap.bat");
}

static std::string whatNewDeviceWasCreated(
    std::vector<std::string> previousNames) {
  for (std::string name : getExistingDeviceNames()) {
    if (std::find(previousNames.begin(), previousNames.end(), name) ==
        previousNames.end()) {
      LOG("new device: %s", name.c_str());
      return name;
    }
  }
  LOG("no new device was created?");
  abort();
}

WinTap* WinTap::create(std::string savedDeviceName) {
  WinTap* self = new WinTap();

  auto existingDevices = getExistingDeviceNames();
  self->deviceName = savedDeviceName;
  if (std::find(existingDevices.begin(), existingDevices.end(),
                savedDeviceName) == existingDevices.end()) {
    createNewTuntap();
    self->deviceName = whatNewDeviceWasCreated(existingDevices);
  }

  self->tap_fd = tuntapStart(self->deviceName);
  return self;
}

std::string WinTap::getNetshName() {
  return getNetshNameForGuid(deviceName);
}

string_view WinTap::read(std::string& buffer) {
  return GIL::unlocked<string_view>([&] {
    OVERLAPPED overlapped = {0, 0, 0};
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (ReadFile(tap_fd, &buffer[0], (DWORD)buffer.size(), NULL, &overlapped) ==
        0) {
      if (GetLastError() != ERROR_IO_PENDING) {
        LOG("tap read failed %d", (int)GetLastError());
        assert(false);
      }
    }

    WaitForSingleObject(overlapped.hEvent, INFINITE);
    CloseHandle(overlapped.hEvent);

    int len = overlapped.InternalHigh;
    return string_view(buffer).substr(0, len);
  });
}

void WinTap::write(string_view data) {
  GIL::unlocked<void>([&] {
    OVERLAPPED overlapped = {0, 0, 0};
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (WriteFile(tap_fd, data.data(), (DWORD)data.size(), NULL, &overlapped)) {
      if (GetLastError() != ERROR_IO_PENDING) {
        LOG("tap write failed");
      }
    }

    WaitForSingleObject(overlapped.hEvent, INFINITE);
    CloseHandle(overlapped.hEvent);
  });
}

void WinTap::bringUp() {
  DWORD len;

  ULONG flag = 1;
  if (DeviceIoControl(tap_fd, TAP_IOCTL_SET_MEDIA_STATUS, &flag, sizeof(flag),
                      &flag, sizeof(flag), &len, NULL) == 0) {
    LOG("failed to bring up the tap device");
    return;
  }

  LOG("tap config OK");
}

std::string WinTap::getMac() {
  std::string hwaddr;
  hwaddr.resize(6);
  DWORD len;

  if (DeviceIoControl(tap_fd, TAP_IOCTL_GET_MAC, &hwaddr[0], hwaddr.size(),
                      &hwaddr[0], hwaddr.size(), &len, NULL) == 0) {
    LOG("failed to retrieve MAC address");
    assert(false);
  } else {
    LOG("MAC address: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x", 0xFF & hwaddr[0],
        0xFF & hwaddr[1], 0xFF & hwaddr[2], 0xFF & hwaddr[3], 0xFF & hwaddr[4],
        0xFF & hwaddr[5]);
  }
  return hwaddr;
}
