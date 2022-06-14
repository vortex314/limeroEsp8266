#include "LedBlinker.h"

LedBlinker::LedBlinker(Thread &thr, uint32_t pin, uint32_t delay)
    : Actor(thr), _ledGpio(DigitalOut::create(pin)), blinkTimer(thr, delay, true)
{
}
void LedBlinker::init()
{
    blinkTimer >> ([&](const TimerMsg tm)
                   {
        _ledGpio.write(_on);
        _on = _on ? 0 : 1 ; });

    blinkSlow >> [&](bool flag)
    {
        if (flag)
            blinkTimer.interval(1000);
        else
            blinkTimer.interval(100);
    };
    pulse >> [&](const bool &b)
    {
        blinkTimer.repeat(false);
        blinkTimer.interval(100);
        _ledGpio.write(0);
        _on = 1;
        blinkTimer.start();
    };
    _ledGpio.init();
    _ledGpio.write(1);
    _on = 0;
}

void LedBlinker::delay(uint32_t d)
{
    blinkTimer.interval(d);
}

void LedBlinker::on()
{
    _ledGpio.write(0);
    _on = 1 ;
}

void LedBlinker::off()
{
    _ledGpio.write(1);
    _on = 0 ;
}
