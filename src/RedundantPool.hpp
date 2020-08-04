#include <atomic>
#include <queue>
#include <myConfig.hpp>
#include <Ethernet.h>
using namespace std;
class PoolDevice
{
public: //http://www.esp32learning.com/code/esp32-true-random-number-generator-example.php
    bool IsMaster;
    IPAddress Address;
    uint32_t Health;
    uint32_t RandomFactor;
    bool Vote;//when a pool device is recived from another device this will be true if the remote device is voting for self to be master
    //when transiting this will be high when self is voting for remote to be master
    //in order for self to be master all other devices must be in agreeement ie. health or random factor are the largest in the pool
    uint8_t FailedContacts =0;// not valid for self
    int64_t LastContact = 0;  // not valid for self
    String version ="";
    static const uint8_t OBJID = POOL_DEVICE_TRANSMIT_ID;
    PoolDevice(IPAddress);
    PoolDevice();
    int Transmit_Size();
    int Transmit_Prep(uint8_t *,bool);
    int From_Transmition(uint8_t *, uint16_t);
};

class PoolManagment
{
public:
    static std::atomic<bool> broadcastIP;
    static std::atomic<bool> ControlRoomFound;
    static std::atomic<bool> InUpdateMode;
    static std::atomic<bool> CreateNewPool;
    static std::atomic<bool> ContactedByPool;
    std::atomic<bool> validPool;
    PoolDevice self;
    deque<PoolDevice *> pool;

    IPAddress UDPBroadcastAddress;
    IPAddress SelfStartingAddress;
    uint16_t UDPport;
    EthernetUDP Udp;
    IPAddress ControlRoomIP;
    PoolManagment(IPAddress, IPAddress, uint16_t); //
    void SendPoolInfo(IPAddress,bool);
    void sendUDPBroadcast();
    void CheckUDPServer();
    void UDPSetup();
    void broadcastMessage(const char *);
    bool createPool();
    void stopBroadcasts(IPAddress);
    void FowardDataToControlRoom(uint8_t *data, size_t DataLength);
};
