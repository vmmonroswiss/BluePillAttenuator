
#include "hmc472.h"

//=============================================================================
HMC472::HMC472(const hmc472_gpio_t* p_gpio)
//=============================================================================
{
    this->p_gpio = p_gpio;
    for(uint8_t i = 0; i< HMC472_IC_QTY; i++)
    {
        int32_t* ptr = (int32_t*)&p_gpio[i];
        for(uint8_t j = 0; j < 6; j++)
        {
            pinMode(*ptr, OUTPUT);
            digitalWrite(*ptr, LOW);
            ptr++;
        }
    }
}


//=============================================================================
void HMC472::SetIndex(int index)
//=============================================================================
{
    if(index < GetMinIndex()) index = GetMinIndex();
    if(index > GetMaxIndex()) index = GetMaxIndex();

    if(this->m_index != index)
    {
        this->m_index = index;
        int value_a = this->m_index >> 1;
        int value_b = this->m_index  - value_a;

        int32_t* ptr = (int32_t*)&p_gpio[0];
        for(uint8_t j = 0; j < 6; j++)
        {
            digitalWrite(*ptr++, (value_a & (1<<j)) ? HIGH: LOW);
        }

        ptr = (int32_t*)&p_gpio[1];
        for(uint8_t j = 0; j < 6; j++)
        {
            digitalWrite(*ptr++, (value_b & (1<<j)) ? HIGH: LOW);
        }
    }
}




//=============================================================================
float HMC472::GetValueDb()
//=============================================================================
{
    return this->m_index*0.5F;
}