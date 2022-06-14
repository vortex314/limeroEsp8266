#include <Udp.h>

#define UDP_IP_ADDR "192.168.0.197"
#define UDP_PORT 9001

Udp::Udp(Thread& thr):Actor(thr) {

}

int Udp::init(){
    int ip_protocol = IPPROTO_IP;
    int addr_family = AF_INET;

    _socket = socket(addr_family, SOCK_DGRAM, ip_protocol);
    if (_socket < 0) {
        ERROR("Unable to create socket: errno %d", errno);
        return errno;
    }
        return 0;
}

int Udp::port(uint16_t port){
    _srcPort = port;
    return 0;
}

void Udp::destination(const char* ip, uint16_t port){
    char addr_str[128];

    _destAddr.sin_addr.s_addr = inet_addr(UDP_IP_ADDR);
    _destAddr.sin_family = AF_INET;
    _destAddr.sin_port = htons(UDP_PORT);

        inet_ntoa_r(_destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
}

int Udp::send(Bytes& bytes){
    int ret;
    ret = sendto(_socket, bytes.data(), bytes.size(), 0, (struct sockaddr*)&_destAddr, sizeof(_destAddr));
    if (ret < 0) {
        ERROR("Unable to send data: errno %d", errno);
        return -1;
    }
    return ret;
}

int Udp::recv(Bytes& bytes){
    int ret;
    socklen_t addr_len;
    addr_len = sizeof(_destAddr);
    ret = recvfrom(_socket, bytes.data(), bytes.size(), 0, (struct sockaddr*)&_destAddr, &addr_len);
    if (ret < 0) {
        ERROR("Unable to receive data: errno %d", errno);
        return -1;
    }
    return ret;
}