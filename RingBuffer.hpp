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

  //TODO: поменять функцию getAverage (сделать вывод кешированного значения)

  T getAverage()
  {
    //TODO: добавить прерывание которое будет вызываться с постоянной переодичностью
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
