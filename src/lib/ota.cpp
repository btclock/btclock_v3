#include "ota.hpp"

TaskHandle_t taskOtaHandle = NULL;
bool isOtaUpdating = false;
QueueHandle_t otaQueue;



void setupOTA()
{
  if (preferences.getBool("otaEnabled", DEFAULT_OTA_ENABLED))
  {
    ArduinoOTA.onStart(onOTAStart);

    ArduinoOTA.onProgress(onOTAProgress);
    ArduinoOTA.onError(onOTAError);
    ArduinoOTA.onEnd(onOTAComplete);

    ArduinoOTA.setHostname(getMyHostname().c_str());
    ArduinoOTA.setMdnsEnabled(false);
    ArduinoOTA.setRebootOnSuccess(false);
    ArduinoOTA.begin();
    // downloadUpdate();
    otaQueue = xQueueCreate(1, sizeof(UpdateMessage));

    xTaskCreate(handleOTATask, "handleOTA", 8192, NULL, 20,
                &taskOtaHandle);
  }
}

void onOTAProgress(unsigned int progress, unsigned int total)
{
  uint percentage = progress / (total / 100);
  pixels.fill(pixels.Color(0, 255, 0));
  if (percentage < 100)
  {
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  }
  if (percentage < 75)
  {
    pixels.setPixelColor(1, pixels.Color(0, 0, 0));
  }
  if (percentage < 50)
  {
    pixels.setPixelColor(2, pixels.Color(0, 0, 0));
  }
  if (percentage < 25)
  {
    pixels.setPixelColor(3, pixels.Color(0, 0, 0));
  }
  pixels.show();
}

void onOTAStart()
{
  forceFullRefresh();
  std::array<String, NUM_SCREENS> epdContent = {"U", "P", "D", "A",
                                                "T", "E", "!"};
  setEpdContent(epdContent);
  // Stop all timers
  esp_timer_stop(screenRotateTimer);
  esp_timer_stop(minuteTimer);
  isOtaUpdating = true;
  // Stop or suspend all tasks
  //  vTaskSuspend(priceUpdateTaskHandle);
  //    vTaskSuspend(blockUpdateTaskHandle);
  vTaskSuspend(workerTaskHandle);
  vTaskSuspend(taskScreenRotateTaskHandle);

//  vTaskSuspend(ledTaskHandle);
  vTaskSuspend(buttonTaskHandle);

  // stopWebServer();
  stopBlockNotify();
  stopPriceNotify();
}

void handleOTATask(void *parameter)
{
  UpdateMessage msg;

  for (;;)
  {
    if (xQueueReceive(otaQueue, &msg, 0) == pdTRUE)
    {
      if (msg.updateType == UPDATE_ALL) {
        int resultWebUi = downloadUpdateHandler(UPDATE_WEBUI);
        int resultFw = downloadUpdateHandler(UPDATE_FIRMWARE);

        if (resultWebUi == 0 && resultFw == 0) {
          ESP.restart();
        } else {
          queueLedEffect(LED_FLASH_ERROR);
          vTaskDelay(pdMS_TO_TICKS(3000));
          ESP.restart();
        }
      }
    }

    ArduinoOTA.handle(); // Allow OTA updates to occur
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

ReleaseInfo getLatestRelease(const String &fileToDownload)
{
  String releaseUrl = "https://api.github.com/repos/btclock/btclock_v3/releases/latest";
  WiFiClientSecure client;
  client.setCACert(github_root_ca);
  HTTPClient http;
  http.begin(client, releaseUrl);
  http.setUserAgent(USER_AGENT);

  int httpCode = http.GET();

  ReleaseInfo info = {"", ""};

  if (httpCode > 0)
  {
    String payload = http.getString();

    JsonDocument doc;
    deserializeJson(doc, payload);

    JsonArray assets = doc["assets"];

    for (JsonObject asset : assets)
    {
      String assetName = asset["name"].as<String>();
      if (assetName == fileToDownload)
      {
        info.fileUrl = asset["browser_download_url"].as<String>();
      }
      else if (assetName == fileToDownload + ".sha256")
      {
        info.checksumUrl = asset["browser_download_url"].as<String>();
      }

      if (!info.fileUrl.isEmpty() && !info.checksumUrl.isEmpty())
      {
        break;
      }
    }
    Serial.printf("Latest release URL: %s\r\n", info.fileUrl.c_str());
    Serial.printf("Checksum URL: %s\r\n", info.checksumUrl.c_str());
  }
  http.end();
  return info;
}

int downloadUpdateHandler(char updateType)
{
  WiFiClientSecure client;
  client.setCACert(github_root_ca);
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  ReleaseInfo latestRelease;

  switch (updateType)
  {
  case UPDATE_FIRMWARE:
  {
    latestRelease = getLatestRelease(getFirmwareFilename());
  }
  break;
  case UPDATE_WEBUI:
  {
    latestRelease = getLatestRelease("littlefs.bin");
    // updateWebUi(latestRelease.fileUrl, U_SPIFFS);
    // return 0;
  }
  break;
  }


  // First, download the expected SHA256
  String expectedSHA256 = downloadSHA256(latestRelease.checksumUrl);
  if (expectedSHA256.isEmpty())
  {
    Serial.println("Failed to get SHA256 checksum. Aborting update.");
    return false;
  }

  http.begin(client, latestRelease.fileUrl);
  http.setUserAgent(USER_AGENT);

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK)
  {
    int contentLength = http.getSize();
    if (contentLength > 0)
    {
      // Allocate memory to store the firmware
      uint8_t *firmware = (uint8_t *)malloc(contentLength);
      if (!firmware)
      {
        Serial.println(F("Not enough memory to store firmware"));
        return false;
      }

      WiFiClient *stream = http.getStreamPtr();
      size_t bytesRead = 0;
      while (bytesRead < contentLength)
      {
        size_t available = stream->available();
        if (available)
        {
          size_t readBytes = stream->readBytes(firmware + bytesRead, available);
          bytesRead += readBytes;
        }
        yield(); // Allow background tasks to run
      }

      if (bytesRead != contentLength)
      {
        Serial.println("Failed to read entire firmware");
        free(firmware);
        return false;
      }

      // Calculate SHA256
      String calculated_sha256 = calculateSHA256(firmware, contentLength);

      Serial.print("Calculated checksum: ");
      Serial.println(calculated_sha256);
      Serial.print("Expected checksum:   ");
      Serial.println(expectedSHA256);

      if (calculated_sha256 != expectedSHA256)
      {
        Serial.println("Checksum mismatch. Aborting update.");
        free(firmware);
        return false;
      }
      
      Update.onProgress(onOTAProgress);

      if (Update.begin(contentLength, updateType))
      {
        onOTAStart();
        size_t written = Update.write(firmware, contentLength);

        if (written == contentLength)
        {
          Serial.println("Written : " + String(written) + " successfully");
          free(firmware);
        }
        else
        {
          Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?");
          free(firmware);
          return 503;
        }

        if (Update.end())
        {
          Serial.println("OTA done!");
          if (Update.isFinished())
          {
            Serial.println("Update successfully completed. Rebooting.");
//            ESP.restart();
          }
          else
          {
            Serial.println("Update not finished? Something went wrong!");
            free(firmware);
            return 503;
          }
        }
        else
        {
          Serial.println("Error Occurred. Error #: " + String(Update.getError()));
          free(firmware);
          return 503;
        }
      }
      else
      {
        Serial.println("Not enough space to begin OTA");
        free(firmware);
        return 503;
      }
    }
    else
    {
      Serial.println("Invalid content length");
      return 503;
    }
  }
  else
  {
    Serial.printf("HTTP error: %d\n", httpCode);
    return 503;
  }
  http.end();

  return 0;
}

void updateWebUi(String latestRelease, int command)
{
  WiFiClientSecure client;
  client.setCACert(github_root_ca);
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(client, latestRelease);
  http.setUserAgent(USER_AGENT);

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK)
  {
    int contentLength = http.getSize();
    if (contentLength > 0)
    {
      uint8_t *buffer = (uint8_t *)malloc(contentLength);
      if (buffer)
      {
        WiFiClient *stream = http.getStreamPtr();
        size_t written = stream->readBytes(buffer, contentLength);

        if (written == contentLength)
        {
          String expectedSHA256 = "";
          if (command == U_FLASH)
          {
            expectedSHA256 = downloadSHA256(getFirmwareFilename());
            Serial.print("Expected checksum:   ");
            Serial.println(expectedSHA256);
          }

          String calculated_sha256 = calculateSHA256(buffer, contentLength);
          Serial.print("Checksum is ");
          Serial.println(calculated_sha256);
          if ((command == U_FLASH && expectedSHA256.equals(calculated_sha256)) || command == U_SPIFFS)
          {
            Serial.println("Checksum verified. Proceeding with update.");

            Update.onProgress(onOTAProgress);

            if (Update.begin(contentLength, command))
            {
              onOTAStart();

              Update.write(buffer, contentLength);
              if (Update.end())
              {
                Serial.println("Update complete. Rebooting.");
                ESP.restart();
              }
              else
              {
                Serial.println("Error in update process.");
              }
            }
            else
            {
              Serial.println("Not enough space to begin OTA");
            }
          }
          else
          {
            Serial.println("Checksum mismatch. Aborting update.");
          }
        }
        else
        {
          Serial.println("Error downloading firmware");
        }
        free(buffer);
      }
      else
      {
        Serial.println("Not enough memory to allocate buffer");
      }
    }
    else
    {
      Serial.println("Invalid content length");
    }
  }
  else
  {
    Serial.print(httpCode);
    Serial.println("Error on HTTP request");
  }
}

void onOTAError(ota_error_t error)
{
  Serial.println(F("\nOTA update error, restarting"));
  Wire.end();
  SPI.end();
  isOtaUpdating = false;
  delay(1000);
  ESP.restart();
}

void onOTAComplete()
{
  Serial.println(F("\nOTA update finished"));
  Wire.end();
  SPI.end();
  delay(1000);
  ESP.restart();
}

bool getIsOTAUpdating()
{
  return isOtaUpdating;
}

String downloadSHA256(const String &sha256Url)
{
  if (sha256Url.isEmpty())
  {
    Serial.println("Failed to get SHA256 file URL");
    return "";
  }

  WiFiClientSecure client;
  client.setCACert(github_root_ca);
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(client, sha256Url);
  http.setUserAgent(USER_AGENT);

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK)
  {
    String sha256 = http.getString();
    sha256.trim(); // Remove any whitespace or newline characters
    return sha256;
  }
  else
  {
    Serial.printf("Failed to download SHA256 file. HTTP error: %d\n", httpCode);
    return "";
  }
}