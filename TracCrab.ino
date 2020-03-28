#include <Arduino.h>
#line 1 "c:\\Users\\Danil\\OneDrive\\Documents\\AGROM\\Trac_Crab\\RingBuffer.hpp"
#define SAMPLES 10
template <typename T>
class RingBuffer
{
private:
  volatile T data[SAMPLES];
  volatile T avaragedData[SAMPLES];
  volatile T prevValue;
  volatile T prevSample;
  short pos;
  short posForAvarage;

public:
  RingBuffer()
  {
    for (size_t i = 0; i < SAMPLES; ++i)
      data[i] = 0;
    for (size_t i = 0; i < SAMPLES; ++i)
      avaragedData[i] = 0;
    pos = 0;
    posForAvarage = 0;
    prevValue = 0;
  };

  ~RingBuffer(){};

  void update(T value)
  {
    data[(pos + 1) % SAMPLES] = value;
    pos = ++pos % SAMPLES;
    if (pos == 0)
    {
      avaragedData[(posForAvarage + 1) % SAMPLES] = getAverage();
      posForAvarage = (posForAvarage + 1) % SAMPLES;
    }
  };

  void update(/*time in micros*/)
  {
    short next_pos = (pos + 1) % SAMPLES;
    data[next_pos] = micros() - prevValue;
    prevValue = micros();
    pos = next_pos;
    if (pos == 0)
    {
      avaragedData[(posForAvarage + 1) % SAMPLES] = getAverage();
      posForAvarage = (posForAvarage + 1) % SAMPLES;
    }

    /*  Serial.print(data[0]);
    Serial.print(" ");
    Serial.print(data[1]);
    Serial.print(" ");
    Serial.print(data[2]);
    Serial.print(" ");
    Serial.print(data[3]);
    Serial.print(" ");
    Serial.print(data[4]);
    Serial.println();*/
  }

  T getAverage()
  {
    float summ{0};
    for (size_t i = 0; i < SAMPLES; ++i)
      summ += data[i];
    return summ / float(SAMPLES);
  };

  T getMid()
  {
    T newData[SAMPLES];

    for (size_t i = 0; i < SAMPLES; ++i)
      newData[i] += avaragedData[i];

    for (size_t i = 0; i < SAMPLES; i++)
    {
      size_t buf = i;
      for (size_t j = i; j < SAMPLES; j++)
      {
        if (data[buf] > data[j])
          buf = j;
      }
      if (buf != i)
      {
        T tmp = data[i];
        data[i] = data[buf];
        data[buf] = tmp;
      }
    }
    
    if( newData[SAMPLES / 2] < (prevSample * 1.7) )
    {
      prevSample = newData[SAMPLES / 2];
      return prevSample;
    }
    else
       return prevSample;
  };

  void check()
  {
    bool tooLong = (micros() - this->prevValue) > 5000000; //TODO confirm that this value (time between signals) not to big or small
    if (tooLong)
      for (size_t i = 0; i < 5; i++)
        data[i] = 0;
  }
};

class RPMMetr : public RingBuffer<long>
{
public:
  float getRPM(int COUNT_OF_BLADES = 1)
  {
    float calc = (this->getMid() * COUNT_OF_BLADES);
    return calc == 0 ? 0 : (60000000.f / calc); //TODO aproof math of calculation RPM
  }
};

#line 1 "c:\\Users\\Danil\\OneDrive\\Documents\\AGROM\\Trac_Crab\\Trac_crab.ino"
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
//RingBuffer<float> *PTR_LFront;
//RingBuffer<float> *PTR_LBack;

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

//обновляем значение рычага режимов каждые 200мс
void updateChannel()
{
  if (millis() - time_to_update_channel > 200)
  {
    bool Mode_2ws  = digitalRead(DI_MODE_2WS),
         Mode_4ws  = digitalRead(DI_MODE_4WS),
         Mode_crab = digitalRead(DI_MODE_CRAB);

    if ( ( (Mode_2ws + Mode_4ws + Mode_crab) == 3 || (Mode_2ws + Mode_4ws + Mode_crab) == 0 ) &&  controll_state != Controll_state::NO )
    {
      controll_state = Controll_state::NO;
    }
    else  if ( Mode_2ws && controll_state != Controll_state::STATE_2WS )
    {
      controll_state = Controll_state::STATE_2WS;
    }
    else  if ( Mode_4ws && controll_state != Controll_state::STATE_4WS )
    {
      controll_state = Controll_state::STATE_4WS;
    }
    else  if ( Mode_crab && controll_state != Controll_state::STATE_CRAB )
    {
      controll_state = Controll_state::STATE_CRAB;
    }

    time_to_update_channel = millis();
  }
}

void setup()
{

  PTR_RFront = new RingBuffer<float>();
  PTR_RBack  = new RingBuffer<float>();
  //PTR_LFront = new RingBuffer<float>();
  //PTR_LBack  = new RingBuffer();

  //select direction for the first channel
  pinMode(PIN_DRIVER_DIR_CH1, OUTPUT);
  digitalWrite(PIN_DRIVER_DIR_CH1, HIGH);
  //and the seckond channel
  pinMode(PIN_DRIVER_DIR_CH2, OUTPUT);
  digitalWrite(PIN_DRIVER_DIR_CH2, HIGH);

  pinMode(PIN_DRIVER_POWER_CH1, OUTPUT);
  pinMode(PIN_DRIVER_POWER_CH2, OUTPUT);

  //set init value of power equal 50%
  analogWrite( PIN_DRIVER_POWER_CH1, 255/2 ); 
  analogWrite( PIN_DRIVER_POWER_CH2, 255/2 );

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
    PTR_RFront->update( analogRead(PIN_PTR_RFront));
    PTR_RBack->update( analogRead(PIN_PTR_RBack));
    //PTR_RFront->update(analogRead(PIN_PTR_RFront));
    //PTR_LBack->update(analogRead(PIN_PTR_LBack));
    time_to_update_PTRs = millis();
  }

  /*--------------------------------------------------------------------------------------------------
    BUISNESS LOGIC
  --------------------------------------------------------------------------------------------------*/
  // analogRead values go from 0 to 1023, analogWrite values from 0 to 255

  if (millis() - time_to_update_logic > 150)
  {
   
    switch (controll_state)
    {
    case Controll_state::STATE_2WS:
      //analogWrite(PIN_DRIVER_POWER_CH1, 255/2);
      //analogWrite(PIN_DRIVER_POWER_CH2, 255/2); // в режиме 2ws фиксируем колёса прямо 
      break;

    case Controll_state::STATE_4WS:
      //analogWrite(PIN_DRIVER_POWER_CH1, task);
      break;

    case Controll_state::STATE_CRAB:
      //analogWrite(PIN_DRIVER_POWER_CH1, task);
      //analogWrite(PIN_DRIVER_POWER_CH2, task);
      break;

    case Controll_state::NO:
      //analogWrite(PIN_DRIVER_POWER_CH1, 255/2);
      //analogWrite(PIN_DRIVER_POWER_CH2, 255/2);
      break;
    
    default:
      break;
    }
  }

#ifdef monitor
  if (millis() - time_to_print > 40)
  {
    Serial.print("RFront(%):\tRBack(%):\tMode:\n");
    Serial.print(PTR_RFront->getAverage());
    Serial.print("\t");
    Serial.print(PTR_RBack->getAverage());
    Serial.print("\t");

    switch (controll_state)
    {
    case Controll_state::STATE_2WS:
      Serial.print(100);
      Serial.print("\t");
      break;

    case Controll_state::STATE_4WS:
      Serial.print(200);
      Serial.print("\t");
      break;

    case Controll_state::STATE_CRAB:
      Serial.print(300);
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