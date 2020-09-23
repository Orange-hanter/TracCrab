#include "config.h"

#define task_monitor3

#define BIG_BRO                                                                            \
  if (millis() - time_to_print > 500)                                                      \
  {                                                                                        \
    String message = "\t\tRFront(%):" + String(PTR_RFront->getAverage() / 1023.f * 100.f); \
    message += "\t\t RBack(%):" + String(PTR_RBack->getAverage() / 1023.f * 100.f);        \
    message += "\t Mode: ";                                                                \
    Serial.print(message);                                                                 \
    switch (controll_state)                                                                \
    {                                                                                      \
    case Controll_state::STATE_2WS:                                                        \
      Serial.println(String("2ws"));                                                       \
      break;                                                                               \
    case Controll_state::STATE_4WS:                                                        \
      Serial.println(String("4ws"));                                                       \
      break;                                                                               \
    case Controll_state::STATE_CRAB:                                                       \
      Serial.println(String("CRAB"));                                                      \
      break;                                                                               \
    case Controll_state::NO:                                                               \
      Serial.println("No one mode used! Check Cabel");                                     \
      break;                                                                               \
    default:                                                                               \
      break;                                                                               \
    }                                                                                      \
    time_to_print = millis();                                                              \
  }

enum Controll_state
{
  STATE_2WS,
  STATE_4WS,
  STATE_CRAB,
  NO
} controll_state;

float fmap(float x, float in_min, float in_max, float out_min, float out_max)
{
  float result = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  result > 255 ? result = 255 : 0;
  result < 0 ? result = 0 : 0;
  return result;
}

short recalcTask(short persent)
// TODO ТУТ ДОЛЖно быть +-6 вольт при 50%
{
  Serial.print("\t\tPersent:" + String(persent));
  if (persent != zeroRBack)
  {
    persent = persent > maxValuePWM ? maxValuePWM : persent;
    persent = persent < minValuePWM ? minValuePWM : persent;
    return 255.f * (float)persent / 100.f;
  }
  return 255.f * (float)zeroRBack / 100.f;
}

short recalcTask_shift(short shift)
{
  if (task_2 == -1)
  {
    task_2 = zeroRBack;
  }
  if (shift == 0)
  {
    task_2 = zeroRBack;
    return 255.f * zeroRBack / 100.f;
  }

  task_2 += shift;

  task_2 = task_2 > maxValuePWM ? maxValuePWM : task_2;
  task_2 = task_2 < minValuePWM ? minValuePWM : task_2;

  return 255.f * (float)task_2 / 100.f;
}

void setTask(short task_to_set, String source = "")
{
#ifdef task_monitor
  if (source != "")
    Serial.println("Commant to change task send " + source);
#endif
  analogWrite(PIN_DRIVER_POWER_CH1, task_to_set);
  //analogWrite(PIN_DRIVER_POWER_CH2, task_to_set);
}

void PROGRAMM_2WS(float sourceVal, float ctrlVal)
{
  String message2;
  short sign = 1;
  long delta = ctrlVal - zeroRBack;
  message2 = "\tDelta:" + String(delta);
  // в зависимости от знака будет меняться направление
  // больше нуля = право
  // меньше нуля = влево
  if (delta < 0)
    sign = -1;
  delta = abs(delta);
  if (delta <= 5)
    task = recalcTask(zeroRBack);
  else if (delta < 15)
    task = recalcTask(zeroRBack + 7 * sign); // как вариант добавить больше точек
  else if (delta < 20)
    task = recalcTask(zeroRBack + 12.5 * sign);
  else if (delta > 20)
    task = recalcTask(zeroRBack + 16 * sign);

  message2 += "\t\tTask:" + String(task);
  Serial.print(message2);

  setTask(task, "2WS");
}

void PROGRAMM_2WS_sh(float sourceVal, float ctrlVal)
{
  String message2;
  short sign = 1;
  long delta = ctrlVal - zeroRBack;
  message2 = "\tDelta:" + String(delta);
  // в зависимости от знака будет меняться направление
  // больше нуля = право
  // меньше нуля = влево
  if (delta < 0)
    sign = -1;
  delta = abs(delta);
  if (delta <= 5)
    task = recalcTask_shift(0);
  else if (delta < 15)
    task = recalcTask_shift(2 * sign); // как вариант добавить больше точек
  else if (delta < 20)
    task = recalcTask_shift(5 * sign);
  else if (delta > 20)
    task = recalcTask_shift(10 * sign);

  message2 += "\t\tTask:" + String(task);
  Serial.print(message2);

  setTask(task, "2WS");
}

void PROGRAMM_4WS_sh(float sourceVal, float ctrlVal)
{
  String message2;
  short sign = 1;
  long delta = (zeroRFront - sourceVal) + (zeroRBack - ctrlVal);
  message2 = "\tDelta:" + String(delta);
  // в зависимости от знака будет меняться направление
  // больше нуля = право
  // меньше нуля = влево
  if (delta > 0)
    sign = -1;
  delta = abs(delta);
  if (delta <= 5)
    task = recalcTask_shift(0);
  else if (delta < 15)
    task = recalcTask_shift(2 * sign); // как вариант добавить больше точек
  else if (delta < 20)
    task = recalcTask_shift(5 * sign);
  else if (delta > 20)
    task = recalcTask_shift(10 * sign);

  message2 += "\t\tTask:" + String(task);
  Serial.print(message2);

  setTask(task, "4WS");
}

void PROGRAMM_CRAB_sh(float sourceVal, float ctrlVal)
{
  String message2;
  short sign = 1;
  long delta = (zeroRFront - sourceVal) - (zeroRBack - ctrlVal);
  message2 = "\tDelta:" + String(delta);
  // в зависимости от знака будет меняться направление
  // больше нуля = право
  // меньше нуля = влево
  if (delta < 0)
    sign = -1;
  delta = abs(delta);
  if (delta <= 5)
    task = recalcTask_shift(0);
  else if (delta < 15)
    task = recalcTask_shift(2 * sign); // как вариант добавить больше точек
  else if (delta < 20)
    task = recalcTask_shift(5 * sign);
  else if (delta > 20)
    task = recalcTask_shift(10 * sign);

  message2 += "\t\tTask:" + String(task);
  Serial.print(message2);

  setTask(task, "CRAB");
}