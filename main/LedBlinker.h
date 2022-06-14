#ifndef LEDBLINKER_H
#define LEDBLINKER_H

#include <limero.h>
#include <Hardware.h>

class LedBlinker : public Actor
{
    int _on=0;
    DigitalOut& _ledGpio;

public:
    static const int BLINK_TIMER_ID=1;
    TimerSource blinkTimer;
    ValueFlow<TimerMsg> timerHandler;
    ValueFlow<bool> blinkSlow;
    ValueFlow<bool> pulse;
    LedBlinker(Thread& thr,uint32_t pin, uint32_t delay);
    void init();
    void delay(uint32_t d);
    void onNext(const TimerMsg&);
    void toggle();
    void on();
    void off();
};

#endif // LEDBLINKER_H
