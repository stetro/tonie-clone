#include "scanner.h"
#define CONFIG_ENABLE_IRQ_ISR
#include <pn532.h>
#include <stdio.h>

#include "command.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/queue.h"
#include "library.h"

void scanner_task(void* pvParameters) {
  Command command;
  uint8_t uid[7];
  uint8_t uidLength;

  if (!scanner_initialize()) {
    ESP_LOGE(SCANNER_TAG, "Scanner Initialization failed");
    return;
  }
  while (1) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    if (readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength, SCANNER_INTERVAL) && uidLength == 4) {
      ESP_LOGI(SCANNER_TAG, "Found 0x%.2X 0x%.2X 0x%.2X 0x%.2X", uid[0], uid[1], uid[2], uid[3]);
      LibraryEntry* entry = find_library_entry(uid);
      if (entry != NULL) {
        command.type = COMMAND_TYPE_PLAY;
        command.data = entry;
        if (xQueueSend(command_queue, &command, 0) != pdTRUE) {
          ESP_LOGW(SCANNER_TAG, "Could not send queue command");
        }
      }
    } else {
      command.type = COMMAND_TYPE_STOP;
      command.data = NULL;
      if (xQueueSend(command_queue, &command, 0) != pdTRUE) {
        ESP_LOGW(SCANNER_TAG, "Could not send queue command");
      }
    }
  }
}

bool scanner_initialize() {
  if (!init_PN532_I2C()) {
    ESP_LOGE(SCANNER_TAG, "Coud not initialize I2C");
    return false;
  }
  if (!setPassiveActivationRetries(0xFF)) {
    ESP_LOGE(SCANNER_TAG, "Coud not set passive activation retries");
    return false;
  }
  if (!SAMConfig()) {
    ESP_LOGE(SCANNER_TAG, "Coud setup SAM configuration");
    return false;
  }
  ESP_LOGI(SCANNER_TAG, "Scanner Initialized");
  return true;
}