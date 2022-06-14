#ifndef __UDP_H__
#define __UDP_H__
#include <string.h>
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include <limero.h>
class Udp :public  Actor {
    uint16_t _port;
            struct sockaddr_in _destAddr;
            uint16_t _srcPort;
            int _socket;

    public:
    Udp(Thread& thr);
    int port(uint16_t);
    void destination(const char* ip, uint16_t port);
    int init();
    int send(Bytes& bytes);
    int recv(Bytes& bytes);
    int recv(Bytes& bytes, uint32_t timeout);
    int recv(Bytes& bytes, uint32_t timeout, uint16_t port);
    int recv(Bytes& bytes, uint32_t timeout, const char* ip, uint16_t port);
    int recv(Bytes& bytes, uint32_t timeout, const char* ip, uint16_t port, uint16_t maxSize);
    int recv(Bytes& bytes, uint32_t timeout, const char* ip, uint16_t port, uint16_t maxSize, uint16_t timeoutMs);
  

};
#endif // __UDP_H__