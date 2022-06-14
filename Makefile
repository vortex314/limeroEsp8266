PROJECT_NAME := limeroEsp8266
IDF_PATH =$(HOME)/esp/ESP8266_RTOS_SDK
LIMERO = $(HOME)/workspace/limero
EXTRA_COMPONENT_DIRS = $(LIMERO)/esp8266_rtos_sdk $(LIMERO)/limero DWM1000 $(LIMERO)/src 

WIFI_P = ${WIFI_PASS}
WIFI_S = ${WIFI_SSID}

CXXFLAGS += -g -ffunction-sections -fdata-sections -fno-threadsafe-statics 
CXXFLAGS += -std=c++11 -fno-rtti -lstdc++ -fno-exceptions 
CXXFLAGS += -DWIFI_PASS=$(WIFI_P) -DWIFI_SSID=$(WIFI_S) -DESP8266_RTOS_SDK
CXXFLAGS += -DMQTT_HOST="limero.ddns.net" -DMQTT_PORT=1883
CXXFLAGS += -I $(LIMERO)/inc -I$(HOME)/workspace/ArduinoJson/src 
CXXFLAGS += -DMAIN_TREESHAKER -DHOSTNAME=shaker1


ESPBAUD=921600
TTY ?= USB0
SERIAL_PORT ?= /dev/tty$(TTY)
ESPPORT = $(SERIAL_PORT)
SERIAL_BAUD = 115200

include $(IDF_PATH)/make/project.mk

term:
	rm -f $(TTY)_minicom.log
	minicom -D $(SERIAL_PORT) -b $(SERIAL_BAUD) -C $(TTY)_minicom.log

