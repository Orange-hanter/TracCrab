#include <Arduino.h>
#include "RingBuffer.hpp"
#include "config.h"

#define monitor //turn on monitoring via USB

enum Controll_state
{
  STATE_2WS,
  STATE_4WS,
  STATE_CRAB,
  NO
} controll_state;

RingBuffer<float> *PTR_RFront;
RingBuffer<float> *PTR_RBack;

short task;

//-------------------------------------------------------------------------------------------------
//-----------------------------------functions-----------------------------------------------------
//-------------------------------------------------------------------------------------------------

float fmap(float x, float in_min, float in_max, float out_min, float out_max)
{
  float result = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  result > 255 ? result = 255 : 0;
  result < 0 ? result = 0 : 0;
  return result;
}

short getTask(short persent)
{
  persent = persent > 100 ? 100 : persent;
  persent = persent < 0 ? 0 : persent;

  short task = 255.f * (float)persent / 100.f;
  return task;
}

//обновляем значение рычага режимов каждые 200мс
void updateChannel()
{
  if (millis() - time_to_update_channel > 200)
  {
    bool Mode_2ws = digitalRead(DI_MODE_2WS),
         Mode_4ws = digitalRead(DI_MODE_4WS),
         Mode_crab = digitalRead(DI_MODE_CRAB);

    // Serial.print( Mode_2ws );
    // Serial.print("\t");
    // Serial.print( Mode_4ws );
    // Serial.print("\t");
    // Serial.print( Mode_crab );
    // Serial.print("\n");

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

  //set init value of power equal 50%
  analogWrite(PIN_DRIVER_POWER_CH1, getTask(zeroValuePWM));
  analogWrite(PIN_DRIVER_POWER_CH2, getTask(zeroValuePWM));

  pinMode(DI_MODE_2WS, INPUT);
  pinMode(DI_MODE_4WS, INPUT);
  pinMode(DI_MODE_CRAB, INPUT);
  controll_state = Controll_state::NO;

  Serial.begin(115200);
}

//-------------------------------------------------------------------------------------------------
//-----------------------------------MAIN FUNCTION-------------------------------------------------
//-------------------------------------------------------------------------------------------------
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

  /*--------------------------------------------------------------------------------------------------
    BUISNESS LOGIC
      analogRead values go from 0 to 1023, analogWrite values from 0 to 255
  --------------------------------------------------------------------------------------------------*/

  if (millis() - time_to_update_logic > 150)
  {
    long delta = 0;
    short sign = 1;
    float sourceVal = PTR_RFront->getAverage() / 1023.f * 100,
            ctrlVal = PTR_RBack->getAverage() / 1023.f * 100;

    switch (controll_state)    {

      case Controll_state::STATE_2WS:
        delta = ctrlVal - zeroRBack;
        if (delta > 0)
          sign = -1;

        if (abs(delta) < 5)
        {
          task = zeroValuePWM;
          break;
        }
        else if (abs(delta) < 20)
          task = getTask( zeroValuePWM + 12.5 * sign );
        else if (abs(delta) > 20)
          task = getTask( zeroValuePWM + 25 * sign );

        analogWrite(PIN_DRIVER_POWER_CH1, task);
        analogWrite(PIN_DRIVER_POWER_CH2, task);
        break;

      case Controll_state::STATE_4WS:
        //code
        task = getTask(zeroValuePWM);
        task = task > getTask(maxValuePWM) ? getTask(maxValuePWM) : task;
        task = task < getTask(minValuePWM) ? getTask(minValuePWM) : task;
        analogWrite(PIN_DRIVER_POWER_CH1, task);
        analogWrite(PIN_DRIVER_POWER_CH2, task);
        break;

      case Controll_state::STATE_CRAB:
        //code
        task = getTask(zeroValuePWM);
        task = task > getTask(maxValuePWM) ? getTask(maxValuePWM) : task;
        task = task < getTask(minValuePWM) ? getTask(minValuePWM) : task;
        analogWrite(PIN_DRIVER_POWER_CH1, task);
        analogWrite(PIN_DRIVER_POWER_CH2, task);
        break;

      case Controll_state::NO:
        //TODO: Discuss behaviour in this case
        break;

      default:
        break;
    }

    time_to_update_logic = millis();
  }

#ifdef monitor
  if (millis() - time_to_print > 40)
  {
    Serial.print("RFront(%):\tRBack(%):\tMode:\n");
    Serial.print(PTR_RFront->getAverage() / 1023.f * 100.f);
    Serial.print("\t");
    Serial.print(PTR_RBack->getAverage()  / 1023.f * 100.f);
    Serial.print("\t");

    switch (controll_state)
    {
    case Controll_state::STATE_2WS:
      Serial.print(10);
      Serial.print("\t");
      break;

    case Controll_state::STATE_4WS:
      Serial.print(20);
      Serial.print("\t");
      break;

    case Controll_state::STATE_CRAB:
      Serial.print(30);
      Serial.print("\t");
      break;

    case Controll_state::NO:
      Serial.print(0);
      Serial.print("\t");
      Serial.print("No one mode used! Check Cabel");
      break;

    default:
      break;
    }

    time_to_print = millis();
  }
#endif
}
