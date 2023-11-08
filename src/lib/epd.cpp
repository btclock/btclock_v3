#include "epd.hpp"

Native_Pin EPD_CS[NUM_SCREENS] = {
    Native_Pin(2),
    Native_Pin(4),
    Native_Pin(6),
    Native_Pin(10),
    Native_Pin(33),
    Native_Pin(21),
    Native_Pin(17),
#if NUM_SCREENS == 9
    // MCP23X17_Pin(mcp2, 7),
    Native_Pin(-1),
    Native_Pin(-1),
#endif
};
Native_Pin EPD_BUSY[NUM_SCREENS] = {
    Native_Pin(3),
    Native_Pin(5),
    Native_Pin(7),
    Native_Pin(9),
    Native_Pin(37),
    Native_Pin(18),
    Native_Pin(16),
};
MCP23X17_Pin EPD_RESET_MPD[NUM_SCREENS] = {
    MCP23X17_Pin(mcp, 8),
    MCP23X17_Pin(mcp, 9),
    MCP23X17_Pin(mcp, 10),
    MCP23X17_Pin(mcp, 11),
    MCP23X17_Pin(mcp, 12),
    MCP23X17_Pin(mcp, 13),
    MCP23X17_Pin(mcp, 14),
};

Native_Pin EPD_DC = Native_Pin(14);

GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B74::HEIGHT> displays[NUM_SCREENS] = {
    GxEPD2_213_B74(&EPD_CS[0], &EPD_DC, &EPD_RESET_MPD[0], &EPD_BUSY[0]),
    GxEPD2_213_B74(&EPD_CS[1], &EPD_DC, &EPD_RESET_MPD[1], &EPD_BUSY[1]),
    GxEPD2_213_B74(&EPD_CS[2], &EPD_DC, &EPD_RESET_MPD[2], &EPD_BUSY[2]),
    GxEPD2_213_B74(&EPD_CS[3], &EPD_DC, &EPD_RESET_MPD[3], &EPD_BUSY[3]),
    GxEPD2_213_B74(&EPD_CS[4], &EPD_DC, &EPD_RESET_MPD[4], &EPD_BUSY[4]),
    GxEPD2_213_B74(&EPD_CS[5], &EPD_DC, &EPD_RESET_MPD[5], &EPD_BUSY[5]),
    GxEPD2_213_B74(&EPD_CS[6], &EPD_DC, &EPD_RESET_MPD[6], &EPD_BUSY[6]),
#if NUM_SCREENS == 9
    GxEPD2_213_B74(&EPD8_CS, &EPD_DC, &EPD_RESET_MPD[7], &EPD8_BUSY),
    GxEPD2_213_B74(&EPD9_CS, &EPD_DC, &EPD_RESET_MPD[8], &EPD9_BUSY),
#endif
};

std::array<String, NUM_SCREENS> currentEpdContent;
std::array<String, NUM_SCREENS> epdContent;
uint32_t lastFullRefresh[NUM_SCREENS];
TaskHandle_t tasks[NUM_SCREENS];
SemaphoreHandle_t epdUpdateSemaphore[NUM_SCREENS];

int fgColor = GxEPD_WHITE;
int bgColor = GxEPD_BLACK;

#define FONT_SMALL Antonio_SemiBold20pt7b
#define FONT_BIG Antonio_SemiBold90pt7b

void setupDisplays()
{
    for (uint i = 0; i < NUM_SCREENS; i++)
    {
        displays[i].init();
    }

    for (uint i = 0; i < NUM_SCREENS; i++)
    {
        epdUpdateSemaphore[i] = xSemaphoreCreateBinary();
        xSemaphoreGive(epdUpdateSemaphore[i]);

        int *taskParam = new int;
        *taskParam = i;

        xTaskCreate(updateDisplay, "EpdUpd" + char(i), 4096, taskParam, tskIDLE_PRIORITY, &tasks[i]); // create task
    }

    epdContent = {"B",
                  "T",
                  "C",
                  "L",
                  "O",
                  "C",
                  "K"};
    for (uint i = 0; i < NUM_SCREENS; i++)
    {
        xTaskNotifyGive(tasks[i]);
    }

    xTaskCreate(taskEpd, "epd_task", 2048, NULL, tskIDLE_PRIORITY, NULL);
}

void taskEpd(void *pvParameters)
{
    while (1)
    {
        bool updatedThisCycle = false;

        for (uint i = 0; i < NUM_SCREENS; i++)
        {
            if (epdContent[i].compareTo(currentEpdContent[i]) != 0)
            {
                if (!updatedThisCycle)
                {
                    updatedThisCycle = true;
                }

                if (xSemaphoreTake(epdUpdateSemaphore[i], pdMS_TO_TICKS(5000)) == pdTRUE)
                {
                    xTaskNotifyGive(tasks[i]);
                }
                else
                {
                    Serial.println("Couldnt get screen" + String(i));
                }
            }
        }

#ifdef WITH_RGB_LED
        if (updatedThisCycle && preferences.getBool("ledFlashOnUpd", false))
        {
            xTaskNotifyGive(ledHandlerTaskHandle);
        }
#endif

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void setEpdContent(std::array<String, NUM_SCREENS> newEpdContent)
{
    epdContent = newEpdContent;
}

extern "C" void updateDisplay(void *pvParameters) noexcept
{
    const int epdIndex = *(int *)pvParameters;
    delete (int *)pvParameters;

    for (;;)
    {
        // Wait for the task notification
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (epdContent[epdIndex].compareTo(currentEpdContent[epdIndex]) != 0)
        {
            
            displays[epdIndex].init(0, false); // Little longer reset duration because of MCP
            uint count = 0;
            while (EPD_BUSY[epdIndex].digitalRead() == HIGH || count < 10) {
                vTaskDelay(pdMS_TO_TICKS(100));
                if (count >= 9) {
                    displays[epdIndex].init(0, false); 
                }
                count++;
            }

            bool updatePartial = true;

            // Full Refresh every half hour
            if (!lastFullRefresh[epdIndex] || (millis() - lastFullRefresh[epdIndex]) > (preferences.getUInt("fullRefreshMin", 30) * 60 * 1000))
            {
                updatePartial = false;
                lastFullRefresh[epdIndex] = millis();
            }
            if (strstr(epdContent[epdIndex].c_str(), "/") != NULL)
            {
                String top = epdContent[epdIndex].substring(0, epdContent[epdIndex].indexOf("/"));
                String bottom = epdContent[epdIndex].substring(epdContent[epdIndex].indexOf("/") + 1);
                splitText(epdIndex, top, bottom, updatePartial);
            }
            else
            {
                showDigit(epdIndex, epdContent[epdIndex].c_str()[0], updatePartial, &FONT_BIG);
            }

            char tries = 0;
            while (tries < 3)
            {
                if (displays[epdIndex].displayWithReturn(updatePartial))
                {
                    displays[epdIndex].hibernate();
                    currentEpdContent[epdIndex] = epdContent[epdIndex];
                    break;
                }

                vTaskDelay(pdMS_TO_TICKS(100));
                tries++;
            }
        }
        xSemaphoreGive(epdUpdateSemaphore[epdIndex]);
    }
}

void splitText(const uint dispNum, String top, String bottom, bool partial)
{
    displays[dispNum].setRotation(2);
    displays[dispNum].setFont(&FONT_SMALL);
    displays[dispNum].setTextColor(getFgColor());

    // Top text
    int16_t ttbx, ttby;
    uint16_t ttbw, ttbh;
    displays[dispNum].getTextBounds(top, 0, 0, &ttbx, &ttby, &ttbw, &ttbh);
    uint16_t tx = ((displays[dispNum].width() - ttbw) / 2) - ttbx;
    uint16_t ty = ((displays[dispNum].height() - ttbh) / 2) - ttby - ttbh / 2 - 12;

    // Bottom text
    int16_t tbbx, tbby;
    uint16_t tbbw, tbbh;
    displays[dispNum].getTextBounds(bottom, 0, 0, &tbbx, &tbby, &tbbw, &tbbh);
    uint16_t bx = ((displays[dispNum].width() - tbbw) / 2) - tbbx;
    uint16_t by = ((displays[dispNum].height() - tbbh) / 2) - tbby + tbbh / 2 + 12;

    // Make separator as wide as the shortest text.
    uint16_t lineWidth, lineX;
    if (tbbw < ttbh)
        lineWidth = tbbw;
    else
        lineWidth = ttbw;
    lineX = round((displays[dispNum].width() - lineWidth) / 2);

    displays[dispNum].fillScreen(getBgColor());
    displays[dispNum].setCursor(tx, ty);
    displays[dispNum].print(top);
    displays[dispNum].fillRoundRect(lineX, displays[dispNum].height() / 2 - 3, lineWidth, 6, 3, getFgColor());
    displays[dispNum].setCursor(bx, by);
    displays[dispNum].print(bottom);
}

void showDigit(const uint dispNum, char chr, bool partial, const GFXfont *font)
{
    String str(chr);
    displays[dispNum].setRotation(2);
    displays[dispNum].setFont(font);
    displays[dispNum].setTextColor(getFgColor());
    int16_t tbx, tby;
    uint16_t tbw, tbh;
    displays[dispNum].getTextBounds(str, 0, 0, &tbx, &tby, &tbw, &tbh);
    // center the bounding box by transposition of the origin:
    uint16_t x = ((displays[dispNum].width() - tbw) / 2) - tbx;
    uint16_t y = ((displays[dispNum].height() - tbh) / 2) - tby;
    displays[dispNum].fillScreen(getBgColor());
    displays[dispNum].setCursor(x, y);
    displays[dispNum].print(str);
}

int getBgColor()
{
    return bgColor;
}

int getFgColor()
{
    return fgColor;
}

void setBgColor(int color)
{
    bgColor = color;
}

void setFgColor(int color)
{
    fgColor = color;
}

std::array<String, NUM_SCREENS> getCurrentEpdContent()
{
  //  Serial.println("currentEpdContent");

    // for (int i = 0; i < NUM_SCREENS; i++) {
    //     Serial.printf("%d = %s", i, currentEpdContent[i]);
    // }
    return currentEpdContent;
}
