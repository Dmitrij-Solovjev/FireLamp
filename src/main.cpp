/* 28.11.24
 * Firelamp with using FreeRTOS
 * don't forget change INCLUDE_xTaskAbortDelay to 1
 */

#include <Arduino.h>
#include "button.h"
#include "Effects.h"
#include <STM32FreeRTOS.h>

// Define the LED pin is attached
#define LED_PIN LED_BUILTIN
#define BUT1_PIN PB8
#define BUT2_PIN PB9
#define LED_MATRIX_PIN PB6

static bool flag_to_interrupt_button_up = false;
static bool flag_to_interrupt_button_down = false;

Effects eff;
TaskHandle_t OnTimerTaskHandleButton_UP;
TaskHandle_t OnTimerTaskHandleButton_DOWN;

////////////////////////////////////////////////////////////////////////////////////////
//                                   Buttons section                                  //
////////////////////////////////////////////////////////////////////////////////////////

static void OnSinglePressButtonDown(void *arg)
{
  UNUSED(arg);
  eff.Set_Effect_Prev();
}

static void OnLongPressButtonDown(void *arg)
{
  UNUSED(arg);
  eff.Set_Brigtness_Prev();
}

static void OnDoublePressButtonDown(void *arg)
{
  UNUSED(arg);
  eff.Set_Speed_Prev();
}

static void OnSinglePressButtonUp(void *arg)
{
  UNUSED(arg);
  eff.Set_Effect_Next();
}

static void OnLongPressButtonUp(void *arg)
{
  UNUSED(arg);
  eff.Set_Brigtness_Next();
}

static void OnDoublePressButtonUp(void *arg)
{
  UNUSED(arg);
  eff.Set_Speed_Next();
}

Button Button_DOWN(BUT1_PIN, OnSinglePressButtonDown, OnDoublePressButtonDown, OnLongPressButtonDown);
Button Button_UP(BUT2_PIN, OnSinglePressButtonUp, OnDoublePressButtonUp, OnLongPressButtonUp);

static void OnOffFunction(void *arg)
{
  UNUSED(arg);
  eff.Manual_On_Off();
}

Virtual_Button Button_ON_OFF(&Button_DOWN, &Button_UP, OnOffFunction,
                             OnTimerTaskHandleButton_UP,
                             OnTimerTaskHandleButton_DOWN);

////////////////////////////////////////////////////////////////////////////////////////
//                                    Tasks section                                   //
////////////////////////////////////////////////////////////////////////////////////////

static void taskLEDMatrixUpgrade(void *arg)
{
  UNUSED(arg);

  while (1)
  {
    eff.Run();
    vTaskDelay((16L * configTICK_RATE_HZ) / 1000L);
  }
}

static void taskOnTimerIterruptButton_DOWN(void *arg)
{
  UNUSED(arg);
  while (1)
    Button_DOWN.OnTimerIterrupt(OnTimerTaskHandleButton_DOWN);
}

static void taskOnTimerIterruptButton_UP(void *arg)
{
  UNUSED(arg);
  while (1)
    Button_UP.OnTimerIterrupt(OnTimerTaskHandleButton_UP);
}

static void onButtonInterrupt_DOWN()
{
  flag_to_interrupt_button_down = true;
}

static void onButtonInterrupt_UP()
{
  flag_to_interrupt_button_up = true;
}

static void taskInterruptsToTasks(void *arg)
{
  UNUSED(arg);
  while (1)
  {
    if (flag_to_interrupt_button_up)
    {
      Button_UP.OnPressInterrupt(OnTimerTaskHandleButton_UP);
      flag_to_interrupt_button_up = false;
    }
    if (flag_to_interrupt_button_down)
    {
      Button_DOWN.OnPressInterrupt(OnTimerTaskHandleButton_DOWN);
      flag_to_interrupt_button_down = false;
    }
    vTaskDelay(1);
  }
}

////////////////////////////////////////////////////////////////////////////////////////
//                                        Setup                                       //
////////////////////////////////////////////////////////////////////////////////////////
void setup()
{
  Serial.begin(9600);
  portBASE_TYPE s1, s2, s3, s4;

  attachInterrupt(Button_DOWN.pin, onButtonInterrupt_DOWN, CHANGE);
  attachInterrupt(Button_UP.pin, onButtonInterrupt_UP, CHANGE);

  s1 = xTaskCreate(taskLEDMatrixUpgrade, NULL, configMINIMAL_STACK_SIZE, NULL, 3, NULL);
  s2 = xTaskCreate(taskOnTimerIterruptButton_DOWN, NULL, configMINIMAL_STACK_SIZE, NULL, 3, &OnTimerTaskHandleButton_DOWN);
  s3 = xTaskCreate(taskOnTimerIterruptButton_UP, NULL, configMINIMAL_STACK_SIZE, NULL, 3, &OnTimerTaskHandleButton_UP);
  s4 = xTaskCreate(taskInterruptsToTasks, NULL, configMINIMAL_STACK_SIZE, NULL, 3, NULL);

  vTaskSuspend(OnTimerTaskHandleButton_DOWN);
  vTaskSuspend(OnTimerTaskHandleButton_UP);

  Button_UP.Set_Parent(&Button_ON_OFF);
  Button_DOWN.Set_Parent(&Button_ON_OFF);

  // check for creation errors
  if (s1 != pdPASS || s2 != pdPASS || s3 != pdPASS || s4 != pdPASS)
  {
    Serial.println(F("Creation problem"));
    while (1);
  }

  vTaskStartScheduler();
  Serial.println("Insufficient RAM");
  while (1);
}

void loop() {}