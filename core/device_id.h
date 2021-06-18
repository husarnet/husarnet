#ifndef DEVICE_ID_H
#define DEVICE_ID_H
using DeviceId = fstring<16>;

static const DeviceId BadDeviceId = DeviceId("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"); //__attribute__((unused));
#endif
