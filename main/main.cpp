#ifdef GENERIC_TREESHAKER
#include <cstring>
#include <stdio.h>
//#include <esp/uart.h>
//#include <espressif/esp_system.h>
#include <limero.h>
#include <LedBlinker.h>
#include <Wifi.h>
#include <MqttWifi.h>
#include <StringUtility.h>
#define TREESHAKER 
//#define DWM1000

/******************************************************************************
 * FunctionName : app_main
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/

Log logger;

void vAssertCalled(unsigned long ulLine, const char *const pcFileName)
{
    printf("Assert called on : %s:%lu", pcFileName, ulLine);
}

extern "C" void vApplicationMallocFailedHook()
{
    printf(" malloc failed ! ");
    while (1)
        ;
}



//------------------------------------------------------------------ actors and threads
Thread mainThread("main");
Thread mqttThread("mqtt");
Poller poller(mainThread);
LedBlinker ledBlue(mainThread, 2, UINT32_MAX);
LedBlinker ledRed(mainThread, 16, 1000);
Wifi wifi(mqttThread);
MqttWifi mqtt(mqttThread);
#ifdef TREESHAKER
#include <ConfigFlow.h>
ConfigFlow<int> shakeTime("shaker/shakeTime", 3000);
DigitalOut &triacPin = DigitalOut::create(15);
TimerSource shakeTimer(mainThread, 2000, false);
ValueFlow<bool> triacOn(false);
#endif
//------------------------------------------------------------------ system props
LambdaSource<std::string> systemBuild([]() {
    return __DATE__ " " __TIME__ ;
});
LambdaSource<std::string> systemHostname([]() {
    return Sys::hostname();
});
LambdaSource<bool> systemAlive([]() {
    return true;
});
#include <FreeRTOS.h>
LambdaSource<uint32_t> systemHeap([]() {
    return esp_get_free_heap_size();
});
LambdaSource<uint64_t> systemUptime([]() {
    return Sys::millis();
});

#include <ConfigFlow.h>
ConfigFlow<std::string> configHost("system/host", "unknown");
ConfigFlow<float> configFloat("system/float", 3.141592653);
TimerSource delayTimer(mainThread,6000,false);
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

//-----------------------------------------------------------------------
#include <DWM1000_Tag.h>
#include <DWM1000_Anchor.h>
#include <driver/uart.h>
extern "C" void app_main(void)
{
    while(true) {
    INFO("Starting limeroEsp8266 on %s heap : %d ", Sys::getProcessor(), Sys::getFreeHeap());
}
    uart_set_baudrate(UART_NUM_0, 115200);
#if defined(ANCHOR) || defined(TAG)
    esp_set_cpu_freq(ESP_CPU_FREQ_160M); // need for speed, DWM1000 doesn't wait !
#endif
    Sys::init();
    std::string conf;

#ifdef HOSTNAME
    Sys::hostname(S(HOSTNAME));
#else
    std::string hn = stringFormat( "ESP82-%d", getMac32() & 0xFFFF);
    Sys::hostname(hn.c_str());
#endif

    ledBlue.init();
    ledRed.init();
    wifi.init();
    mqtt.init();
    wifi.connected >> mqtt.wifiConnected;
    mqtt.connected >> poller.connected;
    mqtt.connected >> ledBlue.blinkSlow;
    //--------------------------------------------------------------------- system props
    systemUptime >> mqtt.toTopic<uint64_t>("system/upTime");
    systemHeap >> mqtt.toTopic<uint32_t>("system/heap");
    systemHostname >> mqtt.toTopic<std::string>("system/hostname");
    systemBuild >> mqtt.toTopic<std::string>("system/build");
    systemAlive >> mqtt.toTopic<bool>("system/alive");
    poller(systemUptime)(systemHeap)(systemHostname)(systemBuild)(systemAlive);
    //-----------------------------------------------------------------  WIFI props
    wifi.macAddress >> mqtt.toTopic<std::string>("wifi/mac");
    wifi.ipAddress >> mqtt.toTopic<std::string>("wifi/ip");
    wifi.ssid >> mqtt.toTopic<std::string>("wifi/ssid");
    wifi.rssi >> mqtt.toTopic<int>("wifi/rssi");
    poller(wifi.macAddress)(wifi.ipAddress)(wifi.ssid)(wifi.rssi);
    //------------------------------------------------------------------- console logging


//------------------------------------------------------------------- DWM1000
#ifdef TREESHAKER
    triacPin.init();
    triacPin.write(0);
    mqtt.topic<int>("shaker/shakeTime") == shakeTime;
    mqtt.topic<bool>("shaker/shake") == triacOn;
    triacOn >> [](const bool &b) { triacPin.write(b?1:0);if(b) shakeTimer.start(); };
    shakeTimer >> [](const TimerMsg &tm) { triacOn.on(false);shakeTimer.stop(); };
    shakeTime >> [](const int &t) { if(t>0)  shakeTimer.interval(t); };
    poller(triacOn)(shakeTime);
#endif
#ifdef TAG
    DWM1000_Tag &tag = *(new DWM1000_Tag(mainThread,
                                         Spi::create(12, 13, 14, 15),
                                         DigitalIn::create(4),
                                         DigitalOut::create(5),
                                         getMac32() & 0xFFFF,
                                         (uint8_t *)"ABCDEF"));
    tag.preStart();
    tag.mqttMsg >> mqtt.outgoing;
    tag.blink >> ledBlue.pulse;
    tag.blinks >> mqtt.toTopic<uint32_t>("tag/blinks");
    tag.polls >> mqtt.toTopic<uint32_t>("tag/polls");
    tag.resps >> mqtt.toTopic<uint32_t>("tag/resps");
    tag.finals >> mqtt.toTopic<uint32_t>("tag/finals");
    tag.interruptCount >> mqtt.toTopic<uint32_t>("tag/interrupts");
    tag.errs >> mqtt.toTopic<uint32_t>("tag/errs");
    tag.timeouts >> mqtt.toTopic<uint32_t>("tag/timeouts");
    poller(tag.blinks)(tag.polls)(tag.resps)(tag.finals)(tag.interruptCount)(tag.errs)(tag.timeouts);

#endif
#ifdef ANCHOR
    DWM1000_Anchor &anchor = *(new DWM1000_Anchor(mainThread,
                                                  Spi::create(12, 13, 14, 15),
                                                  DigitalIn::create(4),
                                                  DigitalOut::create(5),
                                                  getMac32() & 0xFFFF,
                                                  (uint8_t *)S(ANCHOR)));
    delayTimer >> [&](const TimerMsg& tm ){ anchor.preStart();};
    delayTimer.start();
    anchor.mqttMsg >> mqtt.outgoing;
    anchor.poll >> ledBlue.pulse;
    anchor.x == mqtt.topic<int32_t>("anchor/x");
    anchor.y == mqtt.topic<int32_t>("anchor/y");
    anchor.blinks >> mqtt.toTopic<uint32_t>("anchor/blinks");
    anchor.polls >> mqtt.toTopic<uint32_t>("anchor/polls");
    anchor.finals >> mqtt.toTopic<uint32_t>("anchor/finals");
    anchor.interruptCount >> mqtt.toTopic<uint32_t>("anchor/interrupts");
    anchor.errs >> mqtt.toTopic<uint32_t>("anchor/errs");
    anchor.timeouts >> mqtt.toTopic<uint32_t>("anchor/timeouts");
    anchor.distanceRef >> mqtt.toTopic<float>("anchor/distance");
    anchor.address >> mqtt.toTopic<uint16_t>("anchor/address");
    poller(anchor.blinks)(anchor.polls)(anchor.finals)(anchor.interruptCount);
    poller(anchor.errs)(anchor.timeouts)(anchor.distanceRef)(anchor.address);

#endif
    mainThread.start(); // wifi init fails if this doesn't end
    mqttThread.start();
}
#endif