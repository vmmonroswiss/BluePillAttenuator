#include <Arduino.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <RotaryEncoder.h>
#include "hmc472.h"
#include "CRC16.h"


// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = PA6;
const int en = PA7;
const int d4 = PB0; 
const int d5 = PB1; 
const int d6 = PB10; 
const int d7 = PB11;

#define GPIO_PIN_ENCODER_A      PA8 
#define GPIO_PIN_ENCODER_B      PA9
#define GPIO_PIN_ENCODER_SW     PB9

#define GPIO_PIN_LED            PC13


typedef enum {
  SM_SEL_NONE,
  SM_SEL_CHA_DB,
  SM_SEL_CHB_DB,

  SM_SEL_LAST
} sm_selection_t;

/* The circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
 * */


const hmc472_gpio_t hmc472_gpio_ch_a[2] = {
  { 
    .gpio_pin_v6 = PB14,
    .gpio_pin_v5 = PB15,
    .gpio_pin_v4 = PA10,
    .gpio_pin_v3 = PA11,
    .gpio_pin_v2 = PA12,
    .gpio_pin_v1 = PA15,
  },
  { 
    .gpio_pin_v6 = PB3,
    .gpio_pin_v5 = PB4,
    .gpio_pin_v4 = PB5,
    .gpio_pin_v3 = PB6,
    .gpio_pin_v2 = PB7,
    .gpio_pin_v1 = PB8,
  },
};

const hmc472_gpio_t hmc472_gpio_ch_b[2] = {
  { 
    .gpio_pin_v6 = PB2,
    .gpio_pin_v5 = PB12,
    .gpio_pin_v4 = PB13,
    .gpio_pin_v3 = PA5,
    .gpio_pin_v2 = PA4,
    .gpio_pin_v1 = PA3,
  },
  { 
    .gpio_pin_v6 = PA2,
    .gpio_pin_v5 = PA1,
    .gpio_pin_v4 = PA0,
    .gpio_pin_v3 = PC15,
    .gpio_pin_v2 = PC14,
    .gpio_pin_v1 = PC13,
  },
};


HMC472 hmc472_ch_a(hmc472_gpio_ch_a);
HMC472 hmc472_ch_b(hmc472_gpio_ch_b);

CRC16 crc;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
RotaryEncoder encoder(GPIO_PIN_ENCODER_A, GPIO_PIN_ENCODER_B, RotaryEncoder::LatchMode::FOUR3);
int sm_selection = SM_SEL_NONE;


//=============================================================================
bool eeprom_read(uint8_t* rd_storage)
//=============================================================================
{
  uint8_t rd_buffer[4];
  uint8_t *ptr = rd_buffer;

  for(uint8_t i = 0; i < sizeof(rd_buffer); i++)
  {
    rd_buffer[i] = EEPROM.read(i);
    
  }
  

  crc.reset();
  crc.add(*ptr++);
  crc.add(*ptr++);
  uint16_t crc16 = crc.calc();
  if( (*ptr++ == (uint8_t)(crc16>>0)) &&  (*ptr++ == (uint8_t)(crc16>>8)))
  {
    rd_storage[0] = rd_buffer[0];
    rd_storage[1] = rd_buffer[1];
    return true;
  }
  return false;
}



//=============================================================================
void eeprom_store()
//=============================================================================
{
  uint8_t rd_buffer[4];
  uint8_t wr_buffer[4] = 
  {
    (uint8_t)hmc472_ch_a.GetIndex(),
    (uint8_t)hmc472_ch_b.GetIndex()
  };
  uint8_t *ptr = wr_buffer;

  if(eeprom_read(rd_buffer))
  {
    if(rd_buffer[0]== wr_buffer[0] && rd_buffer[1]== wr_buffer[1])
      return;
  }


  crc.reset();
  crc.add(*ptr++);
  crc.add(*ptr++);
  uint16_t crc16 = crc.calc();

  *ptr++ = crc16>>0;
  *ptr++ = crc16>>8;

  for(uint8_t i = 0; i < sizeof(wr_buffer); i++)
    EEPROM.write(i, wr_buffer[i]);
}

//=============================================================================
void update_lcd()
//=============================================================================
{
  float value_a = hmc472_ch_a.GetValueDb();
  float value_b = hmc472_ch_b.GetValueDb();

  lcd.setCursor(9, 1);
  lcd.write(sm_selection == SM_SEL_CHA_DB? 0x7E: ' ');
  if(value_a < 10)   lcd.write(' ');
  lcd.print(value_a, 1);
  lcd.print("dB");

  lcd.setCursor(1, 1);
  lcd.write(sm_selection == SM_SEL_CHB_DB? 0x7E: ' ');
  if(value_b < 10)   lcd.write(' ');
  lcd.print(value_b, 1);
  lcd.print("dB");
}



//=============================================================================
void setup() 
//=============================================================================
{
  pinMode(GPIO_PIN_ENCODER_A,  INPUT_PULLUP);  //TIMER 1 channel 1
  pinMode(GPIO_PIN_ENCODER_B,  INPUT_PULLUP);  //TIMER 1 channel 2
  pinMode(GPIO_PIN_ENCODER_SW, INPUT_PULLUP);  //Encoder SWITCH
    //attach interrupt

  pinMode(GPIO_PIN_LED, OUTPUT);
  digitalWrite(GPIO_PIN_LED, LOW);


  uint8_t rd_buffer[2];
  if(eeprom_read(rd_buffer))
  {
    hmc472_ch_a.SetIndex(rd_buffer[0]);
    hmc472_ch_b.SetIndex(rd_buffer[1]);
  }


  lcd.begin(16, 2);
  lcd.print("Antenuator");
  update_lcd();

}


#define SW_EXIT_MILLIS            5000

//=============================================================================
void loop() 
//=============================================================================
{
  static int last_millis     = 0;
  static int last_sw_timeout = 0;
  static int  last_enc_sw  = HIGH;
  static int  last_enc_pos = 0;
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):

  uint32_t curr_millis = millis();
  lcd.setCursor(0, 0);
  // print the number of seconds since reset:
  //lcd.print(millis() / 1000);


  bool change_f = false;
  
  //Handle Button switch
  int enc_sw = digitalRead(GPIO_PIN_ENCODER_SW);
  if(last_enc_sw != enc_sw)
  {
    last_enc_sw = enc_sw;
    if(enc_sw == LOW)
    {
      if(++sm_selection > SM_SEL_LAST)
        sm_selection = SM_SEL_NONE;
        change_f = true;
    }
  }

  //Handle rotation
  encoder.tick(); 
  int enc_pos = encoder.getPosition();


  //Refresh screen
  if(change_f || last_enc_pos != enc_pos)
  {
    switch(sm_selection)
    {
      case SM_SEL_CHA_DB:
        if(last_enc_pos > enc_pos) hmc472_ch_a.Increase();
        if(last_enc_pos < enc_pos) hmc472_ch_a.Decrease();
        last_sw_timeout = SW_EXIT_MILLIS;
        break;

      case SM_SEL_CHB_DB:
        if(last_enc_pos > enc_pos) hmc472_ch_b.Increase();
        if(last_enc_pos < enc_pos) hmc472_ch_b.Decrease();
        last_sw_timeout = SW_EXIT_MILLIS;
        break;

      default: 
        last_sw_timeout = 1;
        break;
    }
    update_lcd();
  }
  last_enc_pos = enc_pos;


  //Check for save data
  int delta = 0;
  if(last_millis > curr_millis)
    delta = 0xFFFFFFFF - last_millis + curr_millis + 1;
  else
    delta = curr_millis - last_millis;
  last_millis = curr_millis;



  if(last_sw_timeout > 0)
  {
    last_sw_timeout -= delta;
    if(last_sw_timeout <= 0)
    {
        sm_selection = SM_SEL_NONE;
        eeprom_store();
        update_lcd();
    }
  }
  
}



