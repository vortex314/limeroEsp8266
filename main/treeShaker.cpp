
#ifdef MAIN_TREESHAKER
#include <limero.h>
#include <driver/uart.h>
#include <StringUtility.h>
#include <LedBlinker.h>
#include <ConfigFlow.h>
#include <Wifi.h>
#include <UdpFrame.h>
#include <RedisSpine.h>

/******************************************************************************
 * FunctionName : app_main
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/

Log logger;
DigitalOut &triacPin = DigitalOut::create(15);

Thread *mainThread;
TimerSource *shakeTimer;
TimerSource *aliveTimer;
LedBlinker *ledBlue;
LedBlinker *ledRed;
Poller *poller;
ValueFlow<bool> *triacOn;
ConfigFlow<int> *shakeTime;
Wifi *wifi;
UdpFrame *udp;
RedisSpine *spine;
LambdaSource<uint32_t> *heapSource;

uint32_t getMac32();

void init()
{
    nvs_handle _nvs = 0;

    if (_nvs != 0)
        return;
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    esp_wifi_set_ps(WIFI_PS_MAX_MODEM); // no power save to lower latency
}

extern "C" void app_main(void)
{
    uart_set_baudrate(UART_NUM_0, 115200);
    init();
    Sys::init();
#ifndef HOSTNAME
    std::string hn = stringFormat("ESP82-%d", getMac32() & 0xFFFF);
    Sys::hostname(hn.c_str());
#else
    Sys::hostname(S(HOSTNAME));
#endif

    mainThread = new Thread("main");
    ledBlue = new LedBlinker(*mainThread, 2, 100);
    ledRed = new LedBlinker(*mainThread, 16, UINT32_MAX);
    poller = new Poller(*mainThread);

    wifi = new Wifi(*mainThread);
    wifi->init();

    udp = new UdpFrame(*mainThread, "192.168.0.240", 9001);
    wifi->connected >> udp->online;
    udp->init();

    spine = new RedisSpine(*mainThread);
    spine->init();
    spine->connected >> ledBlue->blinkSlow;

    udp->rxdFrame >> spine->rxdFrame;
    spine->txdFrame >> udp->txdFrame;

    udp->start();

    shakeTimer = new TimerSource(*mainThread, 1000, true, "shakeTimer");
    aliveTimer = new TimerSource(*mainThread, 10000, true, "aliveTimer");

    triacOn = new ValueFlow<bool>(false);
    shake/interval  = new ConfigFlow<int>("shaker/shakeTime", 3000);
    heapSource = new LambdaSource<uint32_t>([]()
                                            { return Sys::getFreeHeap(); });
    *heapSource >> spine->publisher<uint32_t>("system/heap");

    (*aliveTimer) >> [&](const TimerMsg &)
    {
        INFO(" %s %s heap : %d ", Sys::hostname(), Sys::getProcessor(), Sys::getFreeHeap());
    };

    ledBlue->init();
    ledRed->init();
    *shakeTimer >> [&](const TimerMsg &)
    {
        INFO(" %s %s heap : %d ", Sys::hostname(), Sys::getProcessor(), Sys::getFreeHeap());
    };
    triacPin.init();
    triacPin.write(0);

    spine->subscriber<bool>("shake/trigger") >> triacOn;
    spine->subscriber<int>("shake/interval") >> shakeTime;
    shakeTime >> spine->publisher<int>("shake/interval");

    *triacOn >> [&](const bool &b)
    {
        INFO("triacOn %d", b);
        triacPin.write(b ? 1 : 0);
        if (b)
        {
            shakeTimer->start();
            ledRed->on();
        }
        else
        {
            ledRed->off();
        }
    };
    *shakeTimer >> [&](const TimerMsg &tm)
    { 
        triacOn->on(false);
    shakeTimer->stop(); };
    *shakeTime >> [&](const int &t)
    { if(t>0)  shakeTimer->interval(t); };
    (*poller) (*shakeTime)(*heapSource);
    mainThread->start();
}

#include <esp_system.h>
uint32_t getMac32()
{
    union
    {
        uint8_t macBytes[6];
        uint64_t macInt;
    };
    macInt = 0L;
    esp_efuse_mac_get_default(macBytes);
    return macInt;
}

void vAssertCalled(unsigned long ulLine, const char *const pcFileName)
{
    printf("Assert called on : %s:%lu", pcFileName, ulLine);
    while (1)
        ;
}

extern "C" void vApplicationMallocFailedHook()
{
    printf(" malloc failed ! ");
    while (1)
        ;
}

#endif