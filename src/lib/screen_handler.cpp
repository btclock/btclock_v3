#include "screen_handler.hpp"

TaskHandle_t priceUpdateTaskHandle;
TaskHandle_t blockUpdateTaskHandle;

std::array<String, NUM_SCREENS> taskEpdContent = {"", "", "", "", "", "", ""};
std::string priceString;

void taskPriceUpdate(void *pvParameters)
{
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        unsigned long price = getPrice();
        uint firstIndex = 0;
        if (false) {
            priceString = ("$" + String(price)).c_str();
            
            if (priceString.length() < (NUM_SCREENS)) {
                priceString.insert(priceString.begin(), NUM_SCREENS - priceString.length(), ' ');
                taskEpdContent[0] = "BTC/USD";
                firstIndex = 1;
            }
        } else {
            priceString = String(int(round(1 / float(price) * 10e7))).c_str();

            if (priceString.length() < (NUM_SCREENS)) {
                priceString.insert(priceString.begin(), NUM_SCREENS - priceString.length(), ' ');
                taskEpdContent[0] = "MSCW/TIME";
                firstIndex = 1;
            }
        }

        for (uint i = firstIndex; i < NUM_SCREENS; i++)
        {
            taskEpdContent[i] = priceString[i];
        }

        setEpdContent(taskEpdContent);
    }
}

void taskBlockUpdate(void *pvParameters)
{
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        std::string blockNrString = String(getBlockHeight()).c_str();
        uint firstIndex = 0;
        if (blockNrString.length() < NUM_SCREENS) {
            blockNrString.insert(blockNrString.begin(), NUM_SCREENS - blockNrString.length(), ' ');    
            taskEpdContent[0] = "BLOCK/HEIGHT";
            firstIndex = 1;
        }

        for (uint i = firstIndex; i < NUM_SCREENS; i++)
        {
            taskEpdContent[i] = blockNrString[i];
        }

        setEpdContent(taskEpdContent);
    }
}


void setupTasks()
{
    xTaskCreate(taskPriceUpdate, "updatePrice", 1024, NULL, 1, &priceUpdateTaskHandle);
    xTaskCreate(taskBlockUpdate, "updateBlock", 1024, NULL, 1, &blockUpdateTaskHandle);
}