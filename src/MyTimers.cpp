
#include <freertos/FreeRTOS.h>
#include "freertos/timers.h"
#include <RedundantPool.hpp>
#include <MyTimers.hpp>
//timer definitions
#define NUM_TIMERS 8
TimerHandle_t xTimers[NUM_TIMERS];

uint32_t AZ_id = 0;
uint32_t EL_id = 1;
uint32_t CheckUDP_id = 2;
uint32_t CheckEthernet_id = 3;
uint32_t BroadcastUDP_id = 4;
uint32_t SendDataToControlRoom_id = 5;
uint32_t DHCPRefresh_id = 6;
uint32_t ACCRefresh_id = 7;

std::atomic<bool> measureAZTemp(false);
std::atomic<bool> measureELTemp(false);
std::atomic<bool> CheckUDP(false);
std::atomic<bool> CheckEthernet(false);
std::atomic<bool> BroadcastUDP(false);
std::atomic<bool> SendDataToControlRoom(false);
std::atomic<bool> DHCPRefresh(false);
std::atomic<bool> ACCRefresh(false);

void TimerCallback(TimerHandle_t xTimer)
{
    uint32_t Timer_id = (uint32_t)pvTimerGetTimerID(xTimer);
    //vTimerSetTimerID(xTimer, (void *)ulCount);
    if (Timer_id == AZ_id)
    {
        measureAZTemp = true;
    }
    else if (Timer_id == EL_id)
    {
        measureELTemp = true;
    }
    else if (Timer_id == CheckUDP_id)
    {
        CheckUDP = true;
    }
    else if (Timer_id == CheckEthernet_id)
    {
        CheckEthernet = true;
    }
    else if (Timer_id == BroadcastUDP_id)
    {
        BroadcastUDP = true;
        if (!PoolManagment::broadcastIP)
        {
            //once we no longer need to broadcast over UDB stop the timer to save resourses
            xTimerStop(xTimers[BroadcastUDP_id], 0);
        }
    }
    else if (Timer_id == SendDataToControlRoom_id)
    {
        if (PoolManagment::ControlRoomFound)
        {
            SendDataToControlRoom = true;
        }
    }
    else if (Timer_id == DHCPRefresh_id)
    {
        DHCPRefresh = true;
    }
    else if (Timer_id == ACCRefresh_id)
    {
        ACCRefresh = true;
    }
}

TickType_t CheckUDPTicks = pdMS_TO_TICKS(50);
TickType_t CheckEthernetTicks = pdMS_TO_TICKS(50);
TickType_t BroadcastUDPTicks = pdMS_TO_TICKS(1000);
TickType_t MeasureAZTempTicks = pdMS_TO_TICKS(1000);
TickType_t MeasureELTempTicks = pdMS_TO_TICKS(1000);
TickType_t DataSendTics = pdMS_TO_TICKS(100);
TickType_t DHCPRefreshTicks = pdMS_TO_TICKS(60000);
TickType_t ACCRefreshTicks = pdMS_TO_TICKS(500);

void StartMyTimer(TimerHandle_t timer)
{
    if (timer == NULL)
    {
        //* The timer was not created.
    }
    else
    {
        if (xTimerStart(timer, 0) != pdPASS)
        {
            //* The timer could not be set into the Active state.
        }
    }
}

void startTimersNoPool()
{
    Serial.println("start timers for non master");
    xTimers[CheckUDP_id] = xTimerCreate("CheckUDP", CheckUDPTicks, pdTRUE, (void *)CheckUDP_id, TimerCallback);
    //xTimers[CheckEthernet_id] = xTimerCreate("CheckEthernet", CheckEthernetTicks, pdTRUE, (void *)CheckEthernet_id, TimerCallback);
    xTimers[BroadcastUDP_id] = xTimerCreate("BroadcastUDP", BroadcastUDPTicks, pdTRUE, (void *)BroadcastUDP_id, TimerCallback);
    xTimers[DHCPRefresh_id] = xTimerCreate("DCHPrefresh", DHCPRefreshTicks, pdTRUE, (void *)DHCPRefresh_id, TimerCallback);
    StartMyTimer(xTimers[CheckUDP_id]);
    //StartMyTimer(xTimers[CheckEthernet_id]);
    StartMyTimer(xTimers[BroadcastUDP_id]);
    StartMyTimer(xTimers[DHCPRefresh_id]);
}

void startTransmitingToControlroom()
{
    xTimers[SendDataToControlRoom_id] = xTimerCreate("DataSend", DataSendTics, pdTRUE, (void *)SendDataToControlRoom_id, TimerCallback);
    StartMyTimer(xTimers[SendDataToControlRoom_id]);
}

void startACCRefresh()
{
    xTimers[ACCRefresh_id] = xTimerCreate("ACCRefresh", ACCRefreshTicks, pdTRUE, (void *)ACCRefresh_id, TimerCallback);
    StartMyTimer(xTimers[ACCRefresh_id]);
}

void startMeasuringTemp()
{
    xTimers[AZ_id] = xTimerCreate("MeasureAZTemp", MeasureAZTempTicks, pdTRUE, (void *)AZ_id, TimerCallback);
    xTimers[EL_id] = xTimerCreate("MeasureELTemp", MeasureELTempTicks, pdTRUE, (void *)EL_id, TimerCallback);
    StartMyTimer(xTimers[AZ_id]);
    StartMyTimer(xTimers[EL_id]);
}

void stopACCRefresh()
{
    xTimerStop(xTimers[ACCRefresh_id], 0);
}
void stopTransmitingToControlroom()
{
    xTimerStop(xTimers[SendDataToControlRoom_id], 0);
}
void stopMeasuringTemp()
{
    xTimerStop(xTimers[AZ_id], 0);
    xTimerStop(xTimers[EL_id], 0);
}