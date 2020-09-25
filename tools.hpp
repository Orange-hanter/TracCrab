#include "config.h"

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

short recalcTask_shift(short shift)
{
  if (task_2 == -1 || shift == 0)
  {
    task_2 = zeroRBack;
    return task_2;
  }

  task_2 += shift;
  task_2 = task_K_a * task_2 + task_K_b;

  task_2 = task_2 > maxValuePWM ? maxValuePWM : task_2;
  task_2 = task_2 < minValuePWM ? minValuePWM : task_2;

  return task_2;
}

// в зависимости от знака будет меняться направление
// больше нуля = право
// меньше нуля = влево
void updateTask(short sign = 1)
{
  float _delta = abs(delta);
  if (delta <= 1)
    task = recalcTask_shift(0);
  
  if (millis() - time_to_update_PWM > 200)
  { 
    if (_delta < 15)
      task = recalcTask_shift(3 * sign); // как вариант добавить больше точек
    else if (_delta <= 20)
      task = recalcTask_shift(4 * sign);
    else
      task = recalcTask_shift(6 * sign);

    time_to_update_PWM = millis();
  }
}

void setTask(short task_to_set, String source = "")
{
#if task_monitor
  if (source != "")
    Serial.println("Commant to change task send " + source);
#endif
  if (task_to_set > 255 || task_to_set < 0)
  {
    // TODO: good place for error indication
    Serial.println("");
    Serial.print("ERROR: NOT CORRECT VALUE FOR TASK");
    Serial.println("");
  }

  int recalced = 255.f * (float)task_to_set / 100.f;
  analogWrite(PIN_DRIVER_POWER_CH1, recalced);
  //analogWrite(PIN_DRIVER_POWER_CH2, task_to_set); // commented becouce of seckond channel used like power supply
}

void PROGRAMM_2WS_sh(float sourceVal, float ctrlVal)
{
  delta = ctrlVal - zeroRBack;
  delta > 0 ? updateTask(-1) : updateTask();

  setTask(task, "2WS");
}

void PROGRAMM_4WS_sh(float sourceVal, float ctrlVal)
{
  //if (sourceVal < zeroRFront)
  delta = (zeroRFront - sourceVal) + (zeroRBack - ctrlVal);
  delta = ctrlVal > maxValueRBack ? 0 : delta;
  delta = ctrlVal < minValueRBack ? 0 : delta;
  delta < 0 ? updateTask(-1) : updateTask();

  setTask(task, "4WS");
}

void PROGRAMM_CRAB_sh(float sourceVal, float ctrlVal)
{
  delta = (zeroRFront - sourceVal) - (zeroRBack - ctrlVal);
  delta = ctrlVal > maxValueRBack ? 0 : delta;
  delta = ctrlVal < minValueRBack ? 0 : delta;
  delta > 0 ? updateTask(-1) : updateTask();

  setTask(task, "CRAB");
}

/*

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
*/
