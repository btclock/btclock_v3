#include "led_handler.hpp"

TaskHandle_t ledTaskHandle = NULL;
QueueHandle_t ledTaskQueue = NULL;
Adafruit_NeoPixel pixels(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
unsigned long ledTaskParams;

void ledTask(void *parameter)
{
    while (1)
    {
        if (ledTaskQueue != NULL)
        {
            if (xQueueReceive(ledTaskQueue, &ledTaskParams, portMAX_DELAY) == pdPASS)
            {
                uint32_t oldLights[NEOPIXEL_COUNT];

                // get current state
                for (int i = 0; i < NEOPIXEL_COUNT; i++)
                {
                    oldLights[i] = pixels.getPixelColor(i);
                }

                switch (ledTaskParams)
                {
                case LED_EFFECT_WIFI_CONNECT_ERROR:
                case LED_FLASH_ERROR:
                    blinkDelayColor(250, 3, 255, 0, 0);
                    break;
                case LED_EFFECT_WIFI_CONNECT_SUCCESS:
                case LED_FLASH_SUCCESS:
                    blinkDelayColor(150, 3, 0, 255, 0);
                    break;
                case LED_FLASH_UPDATE:
                    break;
                case LED_FLASH_BLOCK_NOTIFY:
                    blinkDelayTwoColor(250, 3, pixels.Color(224, 67, 0), pixels.Color(8, 2, 0));
                    break;
                case LED_EFFECT_WIFI_WAIT_FOR_CONFIG:
                    blinkDelayTwoColor(100, 1, pixels.Color(8, 161, 236), pixels.Color(156, 225, 240));
                    break;
                case LED_EFFECT_WIFI_CONNECTING:
                    for (int i = NEOPIXEL_COUNT; i >= 0; i--)
                    {
                        for (int j = NEOPIXEL_COUNT; j >= 0; j--)
                        {
                            if (j == i)
                            {
                                pixels.setPixelColor(i, pixels.Color(16, 197, 236));
                            }
                            else
                            {
                                pixels.setPixelColor(j, pixels.Color(0, 0, 0));
                            }
                        }
                        pixels.show();
                        vTaskDelay(pdMS_TO_TICKS(100));
                    }
                    break;
                case LED_EFFECT_PAUSE_TIMER:
                    for (int i = NEOPIXEL_COUNT; i >= 0; i--)
                    {
                        for (int j = NEOPIXEL_COUNT; j >= 0; j--)
                        {
                            uint32_t c = pixels.Color(0, 0, 0);
                            if (i == j)
                                c = pixels.Color(0, 255, 0);
                            pixels.setPixelColor(j, c);
                        }

                        pixels.show();

                        delay(100);
                    }
                    pixels.setPixelColor(0, pixels.Color(255, 0, 0));
                    pixels.show();

                    delay(900);

                    pixels.clear();
                    pixels.show();
                    break;
                case LED_EFFECT_START_TIMER:
                    pixels.clear();
                    pixels.setPixelColor((NEOPIXEL_COUNT - 1), pixels.Color(255, 0, 0));
                    pixels.show();

                    delay(900);

                    for (int i = NEOPIXEL_COUNT; i--; i > 0)
                    {

                        for (int j = NEOPIXEL_COUNT; j--; j > 0)
                        {
                            uint32_t c = pixels.Color(0, 0, 0);
                            if (i == j)
                                c = pixels.Color(0, 255, 0);

                            pixels.setPixelColor(j, c);
                        }

                        pixels.show();

                        delay(100);
                    }

                    pixels.clear();
                    pixels.show();
                    break;
                }

                // revert to previous state
                for (int i = 0; i < NEOPIXEL_COUNT; i++)
                {
                    pixels.setPixelColor(i, oldLights[i]);
                }

                pixels.show();
            }
        }
    }
}

void setupLeds()
{
    pixels.begin();
    if (preferences.getBool("ledTestOnPower", true))
    {
        pixels.setBrightness(preferences.getUInt("ledBrightness", 128));
        pixels.setPixelColor(0, pixels.Color(255, 0, 0));
        pixels.setPixelColor(1, pixels.Color(0, 255, 0));
        pixels.setPixelColor(2, pixels.Color(0, 0, 255));
        pixels.setPixelColor(3, pixels.Color(255, 255, 255));
    }
    else
    {
        pixels.clear();
    }
    pixels.show();
    setupLedTask();
}

void setupLedTask()
{
    ledTaskQueue = xQueueCreate(2, sizeof(char));

    xTaskCreate(ledTask, "LedTask", 2048, NULL, tskIDLE_PRIORITY, &ledTaskHandle);
}

void blinkDelay(int d, int times)
{
    for (int j = 0; j < times; j++)
    {

        pixels.setPixelColor(0, pixels.Color(255, 0, 0));
        pixels.setPixelColor(1, pixels.Color(0, 255, 0));
        pixels.setPixelColor(2, pixels.Color(255, 0, 0));
        pixels.setPixelColor(3, pixels.Color(0, 255, 0));
        pixels.show();
        vTaskDelay(pdMS_TO_TICKS(d));

        pixels.setPixelColor(0, pixels.Color(255, 255, 0));
        pixels.setPixelColor(1, pixels.Color(0, 255, 255));
        pixels.setPixelColor(2, pixels.Color(255, 255, 0));
        pixels.setPixelColor(3, pixels.Color(0, 255, 255));
        pixels.show();
        vTaskDelay(pdMS_TO_TICKS(d));
    }
    pixels.clear();
    pixels.show();
}

void blinkDelayColor(int d, int times, uint r, uint g, uint b)
{
    for (int j = 0; j < times; j++)
    {
        for (int i = 0; i < NEOPIXEL_COUNT; i++)
        {
            pixels.setPixelColor(i, pixels.Color(r, g, b));
        }

        pixels.show();
        vTaskDelay(pdMS_TO_TICKS(d));

        pixels.clear();
        pixels.show();
        vTaskDelay(pdMS_TO_TICKS(d));
    }
    pixels.clear();
    pixels.show();
}

void blinkDelayTwoColor(int d, int times, uint32_t c1, uint32_t c2)
{
    for (int j = 0; j < times; j++)
    {
        for (int i = 0; i < NEOPIXEL_COUNT; i++)
        {
            pixels.setPixelColor(i, c1);
        }
        pixels.show();
        vTaskDelay(pdMS_TO_TICKS(d));

        for (int i = 0; i < NEOPIXEL_COUNT; i++)
        {
            pixels.setPixelColor(i, c2);
        }
        pixels.show();
        vTaskDelay(pdMS_TO_TICKS(d));
    }
    pixels.clear();
    pixels.show();
}

void clearLeds()
{
    preferences.putBool("ledStatus", false);
    pixels.clear();
    pixels.show();
}

void setLights(int r, int g, int b)
{
    setLights(pixels.Color(r, g, b));
}

void setLights(uint32_t color)
{
    preferences.putUInt("ledColor", color);
    preferences.putBool("ledStatus", true);

    for (int i = 0; i < NEOPIXEL_COUNT; i++)
    {
        pixels.setPixelColor(i, color);
    }

    pixels.show();
}

QueueHandle_t getLedTaskQueue()
{
    return ledTaskQueue;
}

bool queueLedEffect(uint effect)
{
    if (ledTaskQueue == NULL)
    {
        return false;
    }

    char flashType = effect;
    xQueueSend(ledTaskQueue, &flashType, portMAX_DELAY);
}