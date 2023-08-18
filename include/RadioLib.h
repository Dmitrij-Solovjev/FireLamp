#include <SPI.h>
#include <RH_RF22.h>

#define DEBUG(a) a

// #include <RH_ReliableDataGram.h>

/// To connect an STM32F411 Discovery board to SI4432 using Arduino and Arduino_STM32
/// connect the pins like this:
///
///                 STM32        SI4432
///                 GND----------GND-\ (ground in)
///                              SDN-/ (shutdown in)
/// interrupt   pin PA3----------NIRQ  (interrupt request out)
///          SS pin PA4----------NSEL  (chip select in)
///         SCK pin PA5----------SCK   (SPI clock in)
///        MOSI pin PA7----------SDI   (SPI Data in)
///        MISO pin PA6----------SDO   (SPI data out)
///                 VDD----------VCC   (3.3V in)
///................................................
///                 GND----------GND-\ (ground in)
typedef uint16_t pin;

class RadioLib
{
	pin _NSEL = 0, _NIRQ = 0;
	RH_RF22 *rf22 = nullptr;

public:
	RadioLib(pin NSEL = PA4, pin NIRQ = PA3) : _NSEL(NSEL), _NIRQ(NIRQ)
	{
		// Singleton instance of the radio driver
		rf22 = new RH_RF22(PA4, PA3);

		// RHReliableDatagram manager(driver, CLIENT_ADDRESS);
	}

	void init() //(uint8_t tx_power = RH_RF22_TXPOW_1DBM, double frequency = 446.0, RH_RF22::ModemConfigChoice modem_config = RH_RF22::GFSK_Rb125Fd125)
	{
		pinMode(LED_BUILTIN, OUTPUT);

		if (!rf22->init())
		{
			while (1)
			{
				DEBUG(Serial.println("init failed"););
				digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
				delay(1000);
			}
		}

		rf22->setTxPower(RH_RF22_TXPOW_1DBM); // RH_RF22_TXPOW_1DBM   - Самый слабый
											  // RH_RF22_TXPOW_2DBM
											  // RH_RF22_TXPOW_5DBM
											  // RH_RF22_TXPOW_8DBM
											  // RH_RF22_TXPOW_11DBM
											  // RH_RF22_TXPOW_14DBM
											  // RH_RF22_TXPOW_17DBM
											  // RH_RF22_TXPOW_20DBM
											  //
		rf22->setFrequency(446.0);			  // частота 466.0мгц, шаг 0.05мгц.
											  // rf22.setFrequency(446.0, 0.1);
											  // тоже, но с автоподстройкой в 100кгц (по умолчанию 0,05)
											  //

		// http://www.airspayce.com/mikem/arduino/RF22/classRF22.html#a76cd019f98e4f17d9ec00e54e5947ca1
		rf22->setModemConfig(RH_RF22::GFSK_Rb2_4Fd36);
		// GFSK_Rb125Fd125 						// GFSK, No Manchester, Rb = 125kbs, Fd = 125kHz.
		// FSK_Rb125Fd125                       // FSK, No Manchester, Rb = 125kbs, Fd = 125kHz.
		// GFSK_Rb2_4Fd36                       // скорость и модуляция, Rb = 2.4kbs, Fd = 36kHz.
		digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
	}

	~RadioLib()
	{
		delete rf22;
	}

	uint8_t GetMessage(uint8_t (&buf)[RH_RF22_MAX_MESSAGE_LEN])
	{
		if (rf22->available())
		{
			uint8_t lenght = sizeof(buf);

			if (rf22->recv(buf, &lenght))
				return 2;
			else
				return 1;
		}
		else
		{
			return 0;
		}
	}

	void SendMessage(String message)
	{
		uint8_t array_len = message.length();
		if (array_len > RH_RF22_MAX_MESSAGE_LEN)
		{
			DEBUG(Serial.println("<--------------Переполнение-------------->"));
		}
		const uint8_t *data = reinterpret_cast<const uint8_t *>(message.c_str());
		rf22->send(data, array_len);
		rf22->waitPacketSent();
		digitalWrite(PC13, !digitalRead(PC13));
	}
};
