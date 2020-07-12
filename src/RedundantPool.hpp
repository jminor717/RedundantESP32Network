#include <atomic>
#include <queue>
using namespace std;
class PoolDevice
{
public: //http://www.esp32learning.com/code/esp32-true-random-number-generator-example.php
    std::atomic<bool> IsMaster;
    IPAddress Address;
    uint32_t Health;
    uint32_t RandomFactor;
    static const uint8_t OBJID = 130;
    PoolDevice(IPAddress);
    PoolDevice();
    int Transmit_Prep(uint8_t *);
    int From_Transmition(uint8_t*);
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

