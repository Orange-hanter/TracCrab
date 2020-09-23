#include <Arduino.h>
#include "RingBuffer.hpp"
#include "tools.hpp"


RingBuffer<float> *PTR_RFront;
RingBuffer<float> *PTR_RBack;


//обновляем значение рычага режимов каждые 300мс
void updateChannel();

void printDebugInfo();

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

#if BOOST_PWM
  //чёрная магия разгона ардуино до 
  TCCR1A = TCCR1A & 0xe0 | 1;
  TCCR1B = TCCR1B & 0xe0 | 0x09; 
#endif

  //set init value of power equal 50%
  setTask(recalcTask_shift(0), "SETUP");
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

  //обновляем значение на потенциометрах каждые 5мс
  if (millis() - time_to_update_PTRs > 5)
  {
    PTR_RFront->update(analogRead(PIN_PTR_RFront));
    PTR_RBack->update(analogRead(PIN_PTR_RBack));

    time_to_update_PTRs = millis();
  }

  /*-------------------------------------------------------------------------------------
    BUISNESS LOGIC
      analogRead values go from 0 to 1023, analogWrite values from 0 to 255
  -------------------------------------------------------------------------------------*/
  if (millis() - time_to_update_logic > 20)
  {
    float sourceVal = PTR_RFront->getAverage() / 1023.f * 100,
          ctrlVal = PTR_RBack->getAverage() / 1023.f * 100;
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
  }

  printDebugInfo();
}

void updateChannel()
{
  if (millis() - time_to_update_channel > 300)
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

void printDebugInfo()
{

  if (millis() - time_to_print > 333)
  {
    String message = "RFront(%):" + String(PTR_RFront->getAverage() / 1023.f * 100.f);
    message += "\t  RBack(%):" + String(PTR_RBack->getAverage() / 1023.f * 100.f);
    message = "\t Delta:" + String(delta);
    message += "\t Task:" + String(task);
    message += "\t Mode: ";

    Serial.print(message);
    switch (controll_state)
    {
    case Controll_state::STATE_2WS:
      Serial.println(String("2ws"));
      break;
    case Controll_state::STATE_4WS:
      Serial.println(String("4ws"));
      break;
    case Controll_state::STATE_CRAB:
      Serial.println(String("CRAB"));
      break;
    case Controll_state::NO:
      Serial.println("No one mode used! Check Cabel");
      break;
    default:
      break;
    }
    time_to_print = millis();
  }
}
