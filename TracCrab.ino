#include <Arduino.h>
#include "RingBuffer.hpp"
#include "tools.hpp"

#define monitor //turn on monitoring via USB

RingBuffer<float> *PTR_RFront;
RingBuffer<float> *PTR_RBack;

//обновляем значение рычага режимов каждые 200мс
void updateChannel()
{
  if (millis() - time_to_update_channel > 200)
  {
    bool Mode_2ws = digitalRead(DI_MODE_2WS),
         Mode_4ws = digitalRead(DI_MODE_4WS),
         Mode_crab = digitalRead(DI_MODE_CRAB);

    if (((Mode_2ws + Mode_4ws + Mode_crab) == 3 || (Mode_2ws + Mode_4ws + Mode_crab) == 0) && controll_state != Controll_state::NO)
    {
      controll_state = Controll_state::NO;
    }
    else if (!Mode_2ws && Mode_4ws && Mode_crab && controll_state != Controll_state::STATE_2WS)
    {
      controll_state = Controll_state::STATE_2WS;
    }
    else if (Mode_2ws && !Mode_4ws && Mode_crab && controll_state != Controll_state::STATE_4WS)
    {
      controll_state = Controll_state::STATE_4WS;
    }
    else if (Mode_2ws && Mode_4ws && !Mode_crab && controll_state != Controll_state::STATE_CRAB)
    {
      controll_state = Controll_state::STATE_CRAB;
    }

    time_to_update_channel = millis();
  }
}

void setup()
{

  PTR_RFront = new RingBuffer<float>();
  PTR_RBack = new RingBuffer<float>();

  pinMode(PIN_PTR_RFront, INPUT_PULLUP);
  pinMode(PIN_PTR_RBack, INPUT_PULLUP);

  //select direction for the first channel
  pinMode(PIN_DRIVER_DIR_CH1, OUTPUT);
  digitalWrite(PIN_DRIVER_DIR_CH1, HIGH);
  //and the seckond channel
  pinMode(PIN_DRIVER_DIR_CH2, OUTPUT);
  digitalWrite(PIN_DRIVER_DIR_CH2, HIGH);

  pinMode(PIN_DRIVER_POWER_CH1, OUTPUT);
  pinMode(PIN_DRIVER_POWER_CH2, OUTPUT);

#ifdef BOOST_PWM
  //чёрная магия разгона ардуино до 
  TCCR1A = TCCR1A & 0xe0 | 1;
  TCCR1B = TCCR1B & 0xe0 | 0x09; 
#endif BOOST_PWM

  //set init value of power equal 50%
  setTask(recalcTask(zeroRBack), "SETUP");
  analogWrite(PIN_DRIVER_POWER_CH2, 255); //канал 2 на полную мощность!!!

  pinMode(DI_MODE_2WS, INPUT);
  pinMode(DI_MODE_4WS, INPUT);
  pinMode(DI_MODE_CRAB, INPUT);
  controll_state = Controll_state::NO;

  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("Programm STARTED!");
}

//-------------------------------------------------------------------------------------------
//-----------------------------------MAIN FUNCTION*-------------------------------------------
//-------------------------------------------------------------------------------------------
void loop()
{
  updateChannel();

  //обновляем значение на потенциометрах каждые 50мс
  if (millis() - time_to_update_PTRs > 50)
  {
    PTR_RFront->update(analogRead(PIN_PTR_RFront));
    PTR_RBack->update(analogRead(PIN_PTR_RBack));

    time_to_update_PTRs = millis();
  }

  /*-------------------------------------------------------------------------------------
    BUISNESS LOGIC
      analogRead values go from 0 to 1023, analogWrite values from 0 to 255
  -------------------------------------------------------------------------------------*/
  if (millis() - time_to_update_logic > 1500)
  {
    long delta = 0;
    short sign = 1;
    float sourceVal = PTR_RFront->getAverage() / 1023.f * 100,
          ctrlVal = PTR_RBack->getAverage() / 1023.f * 100;
    String message2;
    switch (controll_state)
    {

    /*Режим выравнивания задних колёс
                                         zeroRBack                ctrlVal
        задача уменьшить разницу между нулевой точкой и значениями потенциометра
    */
    case Controll_state::STATE_2WS:
      PROGRAMM_2WS_sh(sourceVal, ctrlVal);
      break;

    /*Режим обратного дублирования передних колёс
                                      sourceVal                             ctrlVal
        задача уменьшить разницу между рулём и значениями потенциометра задних колёс со знаком минус
    */
    case Controll_state::STATE_4WS:
      PROGRAMM_4WS_sh(sourceVal, ctrlVal);
      break;

    case Controll_state::STATE_CRAB:
      PROGRAMM_CRAB_sh(sourceVal, ctrlVal);
      break;

    case Controll_state::NO:
      //TODO: Discuss behaviour in this case
      break;

    default:
      break;
    }

    time_to_update_logic = millis();

#ifdef monitor
    BIG_BRO
#endif
  }
}
