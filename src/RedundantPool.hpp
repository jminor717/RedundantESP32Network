#include <atomic>
class PoolDevice
{
public:
    std::atomic<bool> isMaster;
    IPAddress address;

};

class PoolManagment
{
public:
    std::atomic<bool> validPool;
    std::atomic<bool> broadcastIP;
    PoolDevice self;
    PoolDevice *pool;

    IPAddress UDPServer;
    uint16_t UDPport;
    EthernetUDP Udp;
    PoolManagment(IPAddress, IPAddress, uint16_t); //
    void sendUDPBroadcast();
    void CheckUDPServer();
    void UDPSetup();
};
