# warning! this file can't be in directory named 'esp32' due to collision with built-in component 'esp32'

COMPONENT_ADD_INCLUDEDIRS := ../../common ../../common/esp32 ../../core ../../deps/ArduinoJson/src
COMPONENT_SRCDIRS:=.
CXXFLAGS=-std=c++11 -Wall -Wno-sign-compare -mlongcalls
COMPONENT_OBJS:=main.o
