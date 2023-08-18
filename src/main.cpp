/**
 * Название: Огненная лампа
 * Дата: 14.08.2023
 * Функционал: передача и прием сообщений (SI4432)
 * Отображение эффектов, переключение кнопками
 *
 *
 *
 */

#include <Arduino.h>
#include "RadioLib.h"
#include "Effects.h"
#include "button.h"

#define NAME (String) "Часы"

RadioLib RL22;
HardwareTimer timer_effect(TIM2);
Effects MyEff(&timer_effect);

HardwareTimer timer_button0(TIM9);
HardwareTimer timer_button1(TIM1);

Button button0(PA0, &timer_button0, "PA0");
Button button1(PA1, &timer_button1, "PA1");

Virtual_Button on_off_button(&button0, &button1, &MyEff);
void setup()
{
	Serial.begin(9600);
	pinMode(PA2, OUTPUT);
	pinMode(LED_BUILTIN, OUTPUT);

	// Defaults after init are 1DBM, 446.0MHz, 0.05MHz AFC pull-in, modulation FSK_Rb2_4Fd36
	RL22.init();
	button0.init([]()
				 { button0.Clarify_Status_Single(); },
				 []()
				 { button0.Clarify_Status_Double(); },
				 []()
				 { button0.Clarify_Status_Long(); },
				 []() { // одиночное нажатие
					 MyEff.Set_Effect_Prev();
				 },
				 []() { // двойное нажатие
					 MyEff.Set_Brigtness_Prev();
				 },
				 []() {						 // долгое нажатие
					 MyEff.Set_Speed_Prev(); // digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
				 },
				 []() { // Вызов функции в случае нажатия на кнопку
					 button0.Detect_Press_Type();
				 });

	button1.init([]()
				 { button1.Clarify_Status_Single(); },
				 []()
				 { button1.Clarify_Status_Double(); },
				 []()
				 { button1.Clarify_Status_Long(); },
				 []() { // одиночное нажатие
					 MyEff.Set_Effect_Next();
				 },
				 []() { // двойное нажатие
					 MyEff.Set_Brigtness_Next();
				 },
				 []() {						 // долгое нажатие
					 MyEff.Set_Speed_Next(); // digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
				 },
				 []() { // Вызов функции в случае нажатия на кнопку
					 button1.Detect_Press_Type();
				 });

	timer_effect.attachInterrupt([]()
								 { MyEff.Run(); });

	for (int i = 0; i < 5; i++)
	{
		digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
		// digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
		delay(250);
	}

	MyEff.Start_Timer();
}

void loop()
{

	// если есть доступные данные
	String my_text = "";
	if (Serial.available())
	{
		char my_char = 'a';
		while (my_char != '\n')
			if (Serial.available())
			{
				my_char = (char)(Serial.read());
				my_text += my_char; // read the incoming data as string
				Serial.print(my_char);
			}

		my_text = my_text.substring(0, my_text.length() - 2);
		my_text.trim();

		//  отсылаем то, что получили

		Serial.println("(" + NAME + ")-->Отправка:  " + my_text);
		RL22.SendMessage(my_text);
	}

	///////////////////////////////////////////////////////////////////////////
	// Проверка наличия новых сообщений
	uint8_t buf[RH_RF22_MAX_MESSAGE_LEN] = {};

	switch (RL22.GetMessage(buf))
	{
	case 2:
		Serial.println("(" + NAME + ")<--Пришедший пакет ПОЛУЧЕН: ");
		Serial.println((char *)buf);
		break;
	case 1:
		Serial.println("(" + NAME + ")<--Пришедший пакет НЕ ПОЛУЧЕН");
		break;
	case 0:
		// Сообщений нет
		break;
	}
}
