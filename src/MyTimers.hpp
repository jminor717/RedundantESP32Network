#include <atomic>

extern std::atomic<bool> measureAZTemp;
extern std::atomic<bool> measureELTemp;
extern std::atomic<bool> CheckUDP;
extern std::atomic<bool> CheckEthernet;
extern std::atomic<bool> BroadcastUDP;
extern std::atomic<bool> SendDataToControlRoom;
extern std::atomic<bool> DHCPRefresh;
extern std::atomic<bool> ACCRefresh;

void startTimersNoPool();
void startACCRefresh();
void startTransmitingToControlroom();
void startMeasuringTemp();

void stopACCRefresh();
void stopTransmitingToControlroom();
void stopMeasuringTemp();