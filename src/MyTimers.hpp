#include <atomic>

extern std::atomic<bool> measureAZTemp;
extern std::atomic<bool> measureELTemp;
extern std::atomic<bool> CheckUDP;
extern std::atomic<bool> CheckEthernet;
extern std::atomic<bool> BroadcastUDP;
extern std::atomic<bool> SendDataToControlRoom;

void startTimersNoPool();
void startTransmitingToControlroom();
void startMeasuringTemp();