#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "command.h"
#include "player.h"
#include "scanner.h"
#include "sdkconfig.h"
#include "library.h"

static const char* MAIN_TAG = "TONIE_CLONE";

void app_main(void) {
  TaskHandle_t scanner_handle = NULL;
  TaskHandle_t player_handle = NULL;

  esp_log_level_set("*", ESP_LOG_WARN);
  esp_log_level_set(MAIN_TAG, ESP_LOG_INFO);
  esp_log_level_set(SCANNER_TAG, ESP_LOG_INFO);
  esp_log_level_set(PLAYER_TAG, ESP_LOG_INFO);

  command_queue = xQueueCreate(1, sizeof(Command));

  xTaskCreatePinnedToCore(player_task, "Player", 8192, NULL, 1, &player_handle, 0);
  xTaskCreatePinnedToCore(scanner_task, "Scanner", 8192, NULL, 1, &scanner_handle, 1);
  while (1) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}