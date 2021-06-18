# warning! this file can't be in directory named 'esp32' due to collision with built-in component 'esp32'

COMPONENT_ADD_INCLUDEDIRS := ../../common ../../common/esp32 ../../core ../../deps/ArduinoJson/src
COMPONENT_SRCDIRS:=.
CXXFLAGS=-std=c++11 -Wall -Wno-sign-compare -mlongcalls
COMPONENT_OBJS:=base_config.o configmanager.o filestorage.o global_lock.o husarnet.o husarnet_crypto.o husarnet_secure.o ipaddress.o ipaddress_parse.o l2unwrapper.o legacy_filestorage.o network_dev.o port.o self_hosted.o service_helper_common.o sockets.o util.o port_esp32.o husarnet_main.o memory_configtable.o
