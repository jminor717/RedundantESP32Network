#include <atomic>
#include <queue>
using namespace std;
class PoolDevice
{
public:
    std::atomic<bool> isMaster;
    IPAddress address;
    PoolDevice(IPAddress);
    PoolDevice();
};

class PoolManagment
{
public:
    static std::atomic<bool> broadcastIP;
    std::atomic<bool> validPool;
    PoolDevice self;
    deque<PoolDevice*> pool;

    IPAddress UDPBroadcastAddress;
    uint16_t UDPport;
    EthernetUDP Udp;
    PoolManagment(IPAddress, IPAddress, uint16_t); //
    void sendUDPBroadcast();
    void CheckUDPServer();
    void UDPSetup();
    void broadcastMessage(char *);
};
