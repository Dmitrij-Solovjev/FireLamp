#include <Arduino.h>
#ifndef EFFECTS
#include "Effects.h"
#endif
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
	double_state,
	long_state
};
class Button;

class Virtual_Button
{
	Button *but1, *but2 = nullptr;
	bool flag1, flag2 = false;
	Effects *effects = nullptr;

public:
	Virtual_Button(Button *button1, Button *button2, Effects *effects_class);
	void Child_Pressed(Button *button);
	void Child_Unpressed(Button *button);
};

class Button
{
	String name;
	uint32_t pin;
	Virtual_Button *parent = nullptr;
	button_state state = unpressed_state;
	HardwareTimer *timer = nullptr;
	std::function<void()> f_single, f_double, f_long, execute_single, execute_double, execute_long;
	//	bool

public:
	Button(uint32_t button_pin, HardwareTimer *button_timer, const char button_name[] = "") : timer(button_timer),
																							  name(button_name),
																							  pin(button_pin) {}

	// Первые 3 функции нужны для вызова this.Clarify_Status_Long() и this.Clarify_Status_Single().
	// Последние 3 непосредственно вызывают функции управления
	void init(std::function<void()> on_timer_interrupt_plug_single, std::function<void()> on_timer_interrupt_plug_double,
			  std::function<void()> on_timer_interrupt_plug_long, std::function<void()> on_single_click,
			  std::function<void()> on_double_click, std::function<void()> on_long_click, std::function<void()> on_pin_interrupt)
	{
		attachInterrupt(pin, on_pin_interrupt, CHANGE);
		f_single = on_timer_interrupt_plug_single;
		f_double = on_timer_interrupt_plug_double;
		f_long = on_timer_interrupt_plug_long;
		execute_single = on_single_click;
		execute_double = on_double_click;
		execute_long = on_long_click;
	}

	void Setup_Timer(double frequency)
	{
		timer->setPrescaleFactor(2560 * 21 / frequency); // Set prescaler to 2560*21/1 => timer frequency = 168MHz/(2564*21)*frequency = 3125 Hz * frequency (from prediv'd by 1 clocksource of 168 MHz)
		timer->setOverflow(3125);						 // Set overflow to 3125 => timer frequency = 3125Hz * frequency / 3125 = frequency Hz
		timer->refresh();								 // Make register changes take effect
	}

	void Detect_Press_Type()
	{
		timer->pause();

		if (state == unpressed_state)
		{ // Поступило новое событие. Кнопка нажата первый раз (I)
			// Таймер проверки длинного удержания
			parent->Child_Pressed(this);

			timer->attachInterrupt(f_long);
			this->Setup_Timer(1000.0 / long_time);
			timer->resume();

			state = unknown_state;
		}
		else
		{
			parent->Child_Unpressed(this);
			if (state == unknown_state)
			{ // Всего возможно 2 варианта: кнопка отпущена (II) и кнопка нажата в двойном нажатии (III)
				if (digitalRead(pin) == 0)
				{ // Случай (II).
					// Таймер проверки одинарного нажатия
					timer->attachInterrupt(f_single);
					this->Setup_Timer(1000.0 / pause_time);
					timer->resume();
					// Неясно это одиночное или двойное нажатие.
				}
				else
				{ // Случай (III), зафиксированно двойное нажатие
					// Таймер двойного нажатия (повтора)
					execute_double();
					state = double_state;

					timer->attachInterrupt(f_double);
					this->Setup_Timer(1000.0 / repeat_delay_time);
					timer->resume();
				}
			}
			else if (state == double_state or state == long_state)
			{ // Изменился статус у кнопки -> событие закончилось. Случай (IV)
				state = unpressed_state;
			}
			else
			{
				// Где-то ошибка
			}
		}
	}

	void Set_Parent(Virtual_Button *button_parent)
	{
		parent = button_parent;
	}

	void Clarify_Status_Long()
	{
		timer->pause();
		if (digitalRead(pin) == 1 and state == unknown_state)
		{
			// У нас долгое удержание, теперь понятно, выполняем команду
			state = long_state;
			execute_long();

			// Увеличиваем частоту опроса
			this->Setup_Timer(1000.0 / repeat_delay_time);
			timer->resume();
		}
		else if (state == long_state)
		{
			// Продолжается долгое удержание, выполняем команду
			execute_long();
			timer->resume();
		}
	}

	void Clarify_Status_Single()
	{
		timer->pause();
		if (digitalRead(pin) == 0 and state == unknown_state)
		{
			// У нас одиночное короткое нажатие, теперь понятно, выполняем команду

			execute_single();
			state = unpressed_state;
			// Таймер остановлен, кнопка возвращена в исходное состояние.
		}
	}

	void Clarify_Status_Double()
	{
		timer->pause();

		if (state == double_state)
		{
			// Продолжается двойное удержание, выполняем команду
			execute_double();
			timer->resume();
		}
	}
};

Virtual_Button::Virtual_Button(Button *button1, Button *button2, Effects *effects_class) : but1(button1), but2(button2), effects(effects_class)
{
	but1->Set_Parent(this);
	but2->Set_Parent(this);
}

void Virtual_Button::Child_Pressed(Button *button)
{
	if (button == but1)
	{
		flag1 = true;
	}
	else
	{
		flag2 = true;
	}
	if (flag1 and flag2)
	{
		but1->Clarify_Status_Single();
		but2->Clarify_Status_Single();
		effects->On_Off();
	}
}

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