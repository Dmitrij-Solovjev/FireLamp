#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>
#include "Effects.h"
#include <STM32FreeRTOS.h>

enum delay_time
{
	repeat_delay_time = 25,
	pause_time = 150,
	long_time = 250
};

enum button_state
{
	unpressed_state,
	unknown_state,
	single_state,
	after_single_state,
	double_state,
	long_state
};

struct Button;

class Virtual_Button
{
	Button *but1, *but2 = nullptr;
	bool flag1, flag2 = false;
	TaskFunction_t OnSinglePressTaskFunction;
	TaskHandle_t &TaskHandleBut1, &TaskHandleBut2;

public:
	Virtual_Button(	Button *button1,
					Button *button2,
					TaskFunction_t on_single_press,
					TaskHandle_t &task_handle_but1,
					TaskHandle_t &task_handle_but2);
	
	bool Child_Pressed(Button *button);

	void Child_Unpressed(Button *button)
	{
		if (button == but1)
		{
			flag1 = false;
		}
		else
		{
			flag2 = false;
		}
	};
};

struct Button
{
	uint32_t pin;
	bool first_in = false;
	button_state state = unpressed_state;

	Virtual_Button *parent = nullptr;

	TaskFunction_t OnSinglePressTaskFunction, OnDoublePressTaskFunction, OnLongPressTaskFunction;

	Button(uint32_t button_pin,
		   TaskFunction_t on_single_press,
		   TaskFunction_t on_double_press,
		   TaskFunction_t on_long_press) : pin(button_pin),
										   OnSinglePressTaskFunction(on_single_press),
										   OnDoublePressTaskFunction(on_double_press),
										   OnLongPressTaskFunction(on_long_press) {}

	void OnTimerIterrupt(TaskHandle_t &OnTimerTaskHandle)
	{
		// обрабочкик зажатия кнопки							_------*
		if (state == long_state)
		{
			// [x]: Обработка long_press
			OnLongPressTaskFunction(nullptr);

			vTaskDelay(repeat_delay_time);
		}
		// Обрабочкик двойного зажатия кнопки 					_-_----*
		else if (state == double_state)
		{
			// [x]: Обработка double_press
			// Просто обработали двойное нажатие
			OnDoublePressTaskFunction(nullptr);

			vTaskDelay(repeat_delay_time);
		}
		// Ждали задержку 1ого нажатия, получили long press		_--*
		else if (state == unknown_state)
		{
			// [x]: Обработка long_press
			// Зашли в первый раз, поставили таймер
			if (first_in == true)
			{
				first_in = false;
				vTaskDelay(long_time);
			}
			// сработали по таймеру, значит длительное нажатие
			else
			{
				state = long_state;
				OnLongPressTaskFunction(nullptr);

				vTaskDelay(repeat_delay_time);
			}
		}
		// После первого 1ого нажатия ждали задержку			_-__*
		//   и получили одинарное нажатие
		else if (state == after_single_state)
		{
			// [x]: Обработка single_press или double_press
			// Зашли в первый раз, поставили таймер
			if (first_in == true)
			{
				first_in = false;
				vTaskDelay(pause_time);
			}
			// сработали по таймеру, значит одинарное нажатие
			else
			{
				state = single_state;
				OnSinglePressTaskFunction(nullptr);

				state = unpressed_state;
				vTaskSuspend(OnTimerTaskHandle);
			}
		}
		else
		{
			// Ошибка
			// TODO: Обработка ошибки
			// abuse: unpressed_state
			vTaskSuspend(OnTimerTaskHandle);
		}
	}

	void OnPressInterrupt(TaskHandle_t &OnTimerTaskHandle)
	{
		Serial.println(state);
		// останавливаем события с таймером
		vTaskSuspend(OnTimerTaskHandle);
		if (state == unpressed_state)
		{
			if (parent->Child_Pressed(this))
			{
				return;
			}
		}
		else
		{
			parent->Child_Unpressed(this);
		}

		//  Событие нажатия обрабатывается впервые
		if (state == unpressed_state)
		{
			state = unknown_state;
			first_in = true;
			xTaskAbortDelay(OnTimerTaskHandle);
			vTaskResume(OnTimerTaskHandle);
		}
		// Кнопка была нажата и отпущена - одинарное или двойное нажатие
		else if (state == unknown_state)
		{
			state = after_single_state;
			first_in = true;
			xTaskAbortDelay(OnTimerTaskHandle);
			vTaskResume(OnTimerTaskHandle);
		}
		else if (state == after_single_state)
		{
			state = double_state;
			xTaskAbortDelay(OnTimerTaskHandle);
			vTaskResume(OnTimerTaskHandle);
		}

		// Мы поддерживали определенное состояние (двойное или долгое,
		//	 т.к. пользователь не отпускал кнопки) и теперь пользователь
		//   кнопку отпустил - теперь событие нажатия кнопки завершено
		else if (state == double_state or state == long_state)
		{
			state = unpressed_state;
			// [x]: Закончили обработку длительного нажатия
		}
	}

	void Set_Parent(Virtual_Button *button_parent)
	{
		parent = button_parent;
	}
};

Virtual_Button::Virtual_Button(	Button *button1,
								Button *button2,
								TaskFunction_t on_single_press,
								TaskHandle_t &task_handle_but1,
								TaskHandle_t &task_handle_but2):
		but1(button1),
		but2(button2),
		OnSinglePressTaskFunction(on_single_press),
		TaskHandleBut1(task_handle_but1),
		TaskHandleBut2(task_handle_but2)		
{
	but1->Set_Parent(this);
	but2->Set_Parent(this);
}

bool Virtual_Button::Child_Pressed(Button *button)
{
	if (button == but1)
		flag1 = true;
	else
		flag2 = true;

	if (flag1 == true and flag2 == true)
	{
		OnSinglePressTaskFunction(nullptr);
		vTaskSuspend(TaskHandleBut1);
		vTaskSuspend(TaskHandleBut2);
		but1->state = long_state;
		but2->state = long_state;

		flag1 = flag2 = false;
		return true;
	}
	else
	{
		return false;
	}
}

/*
void Virtual_Button::Child_Unpressed(Button *button)
{
	if (button == but1)
	{
		flag1 = false;
	}
	else
	{
		flag2 = false;
	}
}

*/
#endif