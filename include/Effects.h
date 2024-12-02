#ifndef EFFECTS
#define EFFECTS

#include <Adafruit_NeoPixel.h>
#include "colors.h"
#include "db_perlin.hpp"
#include <SPI.h>
#include <STM32FreeRTOS.h>

//#define PIXEL_PIN PB6                            // Пин для подключения адресных светодиодов
#ifndef LED_MATRIX_PIN
    #define LED_MATRIX_PIN PB6                            // Пин для подключения адресных светодиодов
#endif

#define PIXEL_WIDTH 16                           // Ширина матрицы
#define PIXEL_HEIGHT 16                          // Высота матрицы
#define PIXEL_COUNT (PIXEL_WIDTH * PIXEL_HEIGHT) // Количество светодиодов

// С - коеффицент скорости
// S - масштаб
#define COLOR_C 1
#define RAINBOW_C 4
#define RAINBOW_S 250
#define PERLIN_C 0.0002
#define PERLIN_S 2
#define CLEAN_P_C 0.0002
#define CLEAN_P_S 2

#define SKY_C 0.0002
#define SKY_S 1
#define LAVA_C 0.000225
#define LAVA_S 2
#define FOREST_C 0.000275
#define FOREST_S 3

enum effects_status
{
    off,
    alarm,
    on = 250
};

/////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////

Adafruit_NeoPixel strip(PIXEL_COUNT, LED_MATRIX_PIN, NEO_GRB + NEO_KHZ800);

class Effect
{
protected:
    uint8_t scale = 0;
    uint32_t step = 0;
    uint8_t speed = 100;
    double speed_coeff = 0;

public:
    bool dimmable = true;
    bool speed_variable = true;
    bool scalable = true;
    bool changable = true;

    Effect(double _speed_coeff = 0, uint8_t _scale = 0) : scale(_scale), speed_coeff(_speed_coeff)
    {
    }

    virtual void Draw()
    {
        for (uint16_t i = 0; i < strip.numPixels(); i++)
            strip.setPixelColor(i, 0);
        strip.show();
    }

    void Set_Speed(uint8_t _speed)
    {
        speed = _speed;
    }

    uint8_t Get_Speed()
    {
        return speed;
    }

    virtual void Set_Scale(uint8_t _scale)
    {
        scale = _scale;
    }

    uint8_t Get_Scale()
    {
        return scale;
    }

    ~Effect() {}
};

class Color : public Effect
{

public:
    Color() : Effect(COLOR_C) // TO-DO Возможна ошибка
    {
    }

    virtual void Draw() override
    {
        // firstPixelHue = (step++) * (speed + 1);
        uint32_t firstPixelHue = step; // S = U*t
        // Serial.print(firstPixelHue);
        // Serial.print("     ");
        // Serial.println(speed);

        if (firstPixelHue >= 65536) // TO-DO min max stop для alarm
        {
            step = 0;
            firstPixelHue = 0;
        }

        for (uint16_t i = 0; i < strip.numPixels(); i++)
            strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(firstPixelHue)));

        strip.show();
        step += speed * speed_coeff;
    }

    ~Color() {}
};

class ColorALARM : public Effect
{
    uint8_t delay_counter = 0;
    uint8_t chanel = 0;

public:
    ColorALARM() : Effect()
    {
        dimmable = false;
        speed_variable = false;
        scalable = false;
        changable = false;
    }

    void Draw() override
    {
        if (delay_counter >= 209)
        {
            if (chanel < 255)
            {
                chanel++;
                delay_counter = 0;

                for (uint16_t i = 0; i < strip.numPixels(); i++)
                    strip.setPixelColor(i, ((255 << 8) + chanel) << 8);

                strip.show();
                strip.setBrightness(chanel);
            }
        }
        else
            delay_counter++;
    }

    void Set_Scale(uint8_t _scale) override
    {
        step = _scale * 256 / (speed + 1);
    }

    ~ColorALARM() {}
};

class Rainbow : public Effect
{
public:
    Rainbow() : Effect(RAINBOW_C, RAINBOW_S) {}

    void Draw() override
    {

        // uint32_t firstPixelHue = (step++) * speed * 4;
        uint32_t firstPixelHue = step;

        if (firstPixelHue >= 3 * 65536)
        {
            step = 0;
            firstPixelHue = 0;
        }
        //        firstPixelHue/=speed;
        // Hue of first pixel runs 3 complete loops through the color wheel.
        // Color wheel has a range of 65536 but it's OK if we roll over, so
        // just count from 0 to 3*65536. Adding 256 to firstPixelHue each time
        // means we'll make 3*65536/256 = 768 passes through this outer loop:
        for (int i = 0; i < strip.numPixels(); i++)
        { // For each pixel in strip...
            // Offset pixel hue by an amount to make one full revolution of the
            // color wheel (range of 65536) along the length of the strip
            // (strip.numPixels() steps):
            int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels() * 64 / (1 + scale));
            // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
            // optionally add saturation and value (brightness) (each 0 to 255).
            // Here we're using just the single-argument hue variant. The result
            // is passed through strip.gamma32() to provide 'truer' colors
            // before assigning to each pixel:
            strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
        }
        strip.show(); // Update strip with new contents

        step += speed * speed_coeff;
    }

    void Set_Scale(uint8_t _scale) override
    {
        step = _scale * 256 / (speed + 1);
    }

    ~Rainbow() {}
};

class Perlin : virtual public Effect
{
    int8_t direction = 1;
    const uint32_t seed = 0;
    uint32_t step = 1;
    virtual uint32_t func(double color)
    {
        // color = color * 2 * 255;
        color *= 2 * 255; //(-255 ~255)
        uint8_t chanel = color;
        /*
        if (color > 127)
            chanel = 255;
        if (color < -128)
            chanel = 0;
        else
            chanel = (uint8_t)(color + 128);
        */
        if (color >= 0)
            chanel = 255;
        if (color < 0)
            chanel = 0;

        return (((chanel << 8) + chanel) << 8) + chanel;
    }

public:
    Perlin() : Effect(PERLIN_C, PERLIN_S) // Effect(255, 3)
    {
    }

    void Draw() override
    {

        uint8_t frequency = scale;
        uint8_t octaves = 1;

        const double fx = (frequency * 1.0 / PIXEL_WIDTH);
        const double fy = (frequency * 1.0 / PIXEL_HEIGHT);

        // auto const dt = clock.getElapsedTime().asSeconds();
        for (uint8_t y = 0; y < PIXEL_HEIGHT; ++y)
        {
            for (uint8_t x = 0; x < PIXEL_WIDTH; ++x)
            {
                // auto const noise = (db::perlin(double(x) * fx, double(y) * fy, step * 0.25) * 1.0 +
                //                     db::perlin(double(x) * fx, double(y) * fy, step * 0.75) * 0.5) /
                //                    1.5;
                auto const noise = db::perlin(double(x) * fx, double(y) * fy, step * speed_coeff);

                uint32_t chanels = this->func(noise); // TO-DO Возможна ошибка, внимание на RGBW
                strip.setPixelColor(x * 16 + (x % 2) * 15 + y - 2 * y * (x % 2), chanels);
            }
        }

        strip.show(); // Update strip with new contents

        step += speed * direction;

        if (step == 1 or step >= 65535)
        {
            direction = -direction;
            Serial.println("Change target: ");
            Serial.println(step);
        }
    }

    ~Perlin() {}
};

class Clean_Perlin : public Perlin, virtual public Effect
{
public:
    Clean_Perlin() : Effect(CLEAN_P_C, CLEAN_P_S) {}

private:
    uint32_t func(double color)
    {
        auto scale = 5;
        color = ((color * 0.5) * scale) * 255 + 70; //+ 127;
        if (color > 255)
            color = 255;
        if (color < 0)
            color = 0;
        uint8_t chanel = color;
        return (((chanel << 8) + chanel) << 8) + chanel;
    }
};

class Sky : public Perlin, virtual public Effect
{
public:
    Sky() : Effect(SKY_C, SKY_S) {}

private:
    uint32_t func(double color)
    {
        color = color * 255;
        uint8_t chanel = color;
        if (color > 255)
            chanel = 255;
        if (color < 0)
            chanel = 0;
        return CloudColors_p[chanel / 16];
    }
};

class Lava : public Perlin, virtual public Effect
{
public:
    Lava() : Effect(LAVA_C, LAVA_S) {}

private:
    uint32_t func(double color)
    {
        color = color * 255;
        uint8_t chanel = color;
        if (color > 255)
            chanel = 255;
        if (color < 0)
            chanel = 0;
        return LavaColors_p[chanel / 16];
    }
};

class Ocean : public Perlin, virtual public Effect
{
public:
    Ocean() : Effect(LAVA_C, LAVA_S) {}

private:
    uint32_t func(double color)
    {
        color = color * 255;
        uint8_t chanel = color;
        if (color > 255)
            chanel = 255;
        if (color < 0)
            chanel = 0;
        return OceanColors_p[chanel / 16];
    }
};

class Forest : public Perlin, virtual public Effect
{
public:
    Forest() : Effect(FOREST_C, FOREST_S) {}

private:
    uint32_t func(double color)
    {
        color = color * 255;
        uint8_t chanel = color;
        if (color > 255)
            chanel = 255;
        if (color < 0)
            chanel = 0;
        return ForestColors_p[chanel / 16];
    }
};

class Fire : public Effect
{
    // https://github.com/toggledbits/MatrixFireFast
    uint32_t buffer[PIXEL_HEIGHT][PIXEL_WIDTH] = {};
    /* This is the map of colors from coolest (black) to hottest. Want blue flames? Go for it! */
    const uint32_t colors[11] = {
        0x000000,
        0x100000,
        0x300000,
        0x600000,
        0x800000,
        0xA00000,
        0xC02000,
        0xFF2800,
        0xFF3600,
        0xFF4400,
        0xFF5210};
    // 0xFFC030};
    // 0xC08030};

    const uint8_t NCOLORS = (sizeof(colors) / sizeof(colors[0]));

    /* Flare constants */
    const uint8_t flarerows = 2;       /* number of rows (from bottom) allowed to flare */
    const static uint8_t maxflare = 8; /* max number of simultaneous flares */
    const uint8_t flarechance = 50;    /* chance (%) of a new flare (if there's room) */
    const uint8_t flaredecay = 14;     /* decay rate of flare radiation; 14 is good */

    uint8_t nflare = 0;
    uint32_t flare[maxflare] = {};
    bool del_frame = false;

public:
    Fire() {};

    uint32_t isqrt(uint32_t n)
    {
        if (n < 2)
            return n;
        uint32_t smallCandidate = isqrt(n >> 2) << 1;
        uint32_t largeCandidate = smallCandidate + 1;
        return (largeCandidate * largeCandidate > n) ? smallCandidate : largeCandidate;
    }

    // Set pixels to intensity around flare
    void glow(int x, int y, int z)
    {
        int b = z * 10 / flaredecay + 1;
        for (int i = (y - b); i < (y + b); ++i)
        {
            for (int j = (x - b); j < (x + b); ++j)
            {
                if (i >= 0 && j >= 0 && i < PIXEL_WIDTH && j < PIXEL_HEIGHT)
                {
                    int d = (flaredecay * isqrt((x - j) * (x - j) + (y - i) * (y - i)) + 5) / 10;
                    uint8_t n = 0;
                    if (z > d)
                        n = z - d;
                    if (n > buffer[i][j])
                    { // can only get brighter
                        buffer[i][j] = n;
                    }
                }
            }
        }
    }

    void newflare()
    {
        if (nflare < maxflare && random(1, 101) <= flarechance)
        {
            int x = random(0, PIXEL_HEIGHT);
            int y = random(0, flarerows);
            int z = NCOLORS - 1;
            flare[nflare++] = (z << 16) | (y << 8) | (x & 0xff);
            glow(x, y, z);
        }
    }

    void Draw() override
    {
        if (del_frame)
        {
            del_frame = false;
            return;
        }
        del_frame = true;
        uint16_t i,
            j;

        // First, move all existing heat points up the display and fade
        for (i = PIXEL_WIDTH - 1; i > 0; --i)
        {
            for (j = 0; j < PIXEL_HEIGHT; ++j)
            {
                uint8_t n = 0;
                if (buffer[i - 1][j] > 0)
                    n = buffer[i - 1][j] - 1;
                buffer[i][j] = n;
            }
        }

        // Heat the bottom row
        for (j = 0; j < PIXEL_HEIGHT; ++j)
        {
            i = buffer[0][j];
            if (i > 0)
            {
                buffer[0][j] = random(NCOLORS - 6, NCOLORS - 2);
            }
        }

        // flare
        for (i = 0; i < nflare; ++i)
        {
            int x = flare[i] & 0xff;
            int y = (flare[i] >> 8) & 0xff;
            int z = (flare[i] >> 16) & 0xff;
            glow(x, y, z);
            if (z > 1)
            {
                flare[i] = (flare[i] & 0xffff) | ((z - 1) << 16);
            }
            else
            {
                // This flare is out
                for (int j = i + 1; j < nflare; ++j)
                {
                    flare[j - 1] = flare[j];
                }
                --nflare;
            }
        }
        newflare();

        // Set and draw
        for (i = 0; i < PIXEL_WIDTH; ++i)
        {
            for (j = 0; j < PIXEL_HEIGHT; ++j)
            {
                strip.setPixelColor(i * 16 + (i % 2) * 15 + j - 2 * j * (i % 2), colors[buffer[i][j]]);
            }
        }
        strip.show();
    }
};

class Effects
{
    effects_status working_status = on;
#define NUMBER_OF_EFFECTS 9
    Effect *effect[NUMBER_OF_EFFECTS] = {
        new ColorALARM(),
        new Color(),
        new Rainbow(),
        new Clean_Perlin(),
        new Fire(),
        new Sky(),
        new Lava(),
        new Ocean(),
        new Forest()};

    uint8_t current_effect_number, backup_effect_number, brightness;

public:
    Effects(size_t default_brightness = 25,
            size_t effects_count = NUMBER_OF_EFFECTS,
            size_t default_effect_number = 3) : brightness(default_brightness),
                                                current_effect_number(default_effect_number)
    {
        pinMode(LED_MATRIX_PIN, OUTPUT);
        strip.setBrightness(brightness);
    }

    void Run()
    {
        if (working_status == effects_status::off)
            return;
        effect[current_effect_number]->Draw();
    }

    ////////////   Effect select  ////////////
    // Если текущий и выбранный эффект можно переключать, изменение принимается
    void Set_Effect(uint8_t number)
    {
        if (working_status == effects_status::off)
            return;
        if (effect[number]->changable && effect[current_effect_number]->changable)
            current_effect_number = number;
    }

    void Set_Effect_Next()
    {
        if (working_status == effects_status::off)
            return;
        if (current_effect_number < NUMBER_OF_EFFECTS - 1)
            Set_Effect(current_effect_number + 1);
    }

    void Set_Effect_Prev()
    {
        if (working_status == effects_status::off)
            return;
        if (current_effect_number > 0)
            Set_Effect(current_effect_number - 1);
    }

    /////////////   Brightness   /////////////
    // Если текущий эффект можно изменять по яркости, изменение принимается
    void Set_Brigtness(uint8_t _brightness)
    {
        if (working_status == effects_status::off)
            return;
        if (effect[current_effect_number]->dimmable)
            strip.setBrightness(brightness = _brightness);
    }

    void Set_Brigtness_Next()
    {
        if (working_status == effects_status::off)
            return;
        if (brightness < 255)
            Set_Brigtness(brightness + 1);
    }

    void Set_Brigtness_Prev()
    {
        if (working_status == effects_status::off)
            return;
        if (brightness > 0)
            Set_Brigtness(brightness - 1);
    }

    ///////////   Speed variable   ///////////
    // Если текущий эффект можно изменять по скорости, изменение принимается
    void Set_Speed(uint8_t _speed)
    {
        if (working_status == effects_status::off)
            return;
        if (effect[current_effect_number]->speed_variable)
            effect[current_effect_number]->Set_Speed(_speed);
    }

    void Set_Speed_Prev()
    {
        if (working_status == effects_status::off)
            return;
        uint8_t speed = effect[current_effect_number]->Get_Speed();
        if (speed >= 7)
        {
            speed = 0.8 * speed;
            this->Set_Speed(speed);
        }
    }

    void Set_Speed_Next()
    {
        if (working_status == effects_status::off)
            return;
        uint8_t speed = effect[current_effect_number]->Get_Speed();
        if (speed <= 213)
        {
            speed = 1.2 * speed;
            this->Set_Speed(speed);
        }
    }

    ///////////   Speed variable   ///////////
    // Если текущий эффект можно изменять по скорости, изменение принимается
    void Set_Scale(uint16_t _scale)
    {
        if (working_status == effects_status::off)
            return;
        if (effect[current_effect_number]->scalable)
            effect[current_effect_number]->Set_Scale(_scale);
    }

    void Manual_On_Off()
    {
        if (working_status == effects_status::on)
        {
            strip.setBrightness(0);
            strip.show();
            working_status = effects_status::off;
            strip.show();
        }
        else if (working_status == effects_status::off)
        {
            working_status = effects_status::on;
            strip.setBrightness(brightness);
            strip.show();
        } else if (working_status == effects_status::alarm)
        {
            Alarm_Off();
        }
    }

    void Alarm_On()
    {
        if (current_effect_number != 0)
        {
            backup_effect_number = current_effect_number;
            current_effect_number = 0; // Выбрать эффект будильника
            // TODO vTaskSuspend()
        }
    }

    void Alarm_Off()
    {
        current_effect_number = backup_effect_number;
        strip.setBrightness(0);
        strip.show();
        working_status = effects_status::off;
        // TODO: vTaskResume()
    }
};

#endif