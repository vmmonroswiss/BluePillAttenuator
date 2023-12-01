#pragma once
#include <Arduino.h>


typedef struct {
    int32_t gpio_pin_v6;    //0.5dB
    int32_t gpio_pin_v5;    //  1dB
    int32_t gpio_pin_v4;    //  2dB
    int32_t gpio_pin_v3;    //  4dB
    int32_t gpio_pin_v2;    //  8dB
    int32_t gpio_pin_v1;    // 16dB
} hmc472_gpio_t;


#define HMC472_IC_QTY               2
class HMC472 {
public:
    HMC472(const hmc472_gpio_t* gpio);
    int GetMinIndex()   { return 0;                   }
    int GetMaxIndex()   { return 63 * HMC472_IC_QTY;  }
    int GetIndex()      { return m_index; }
    void Increase()     { SetIndex(GetIndex() + 1); }
    void Decrease()     { SetIndex(GetIndex() - 1); }
    void SetIndex(int index);
    float GetValueDb();


private:
    const hmc472_gpio_t* p_gpio;
    int m_index;
};