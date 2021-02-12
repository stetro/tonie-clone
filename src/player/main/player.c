#include "player.h"

#include <string.h>

#include "audio_common.h"
#include "audio_element.h"
#include "audio_event_iface.h"
#include "audio_pipeline.h"
#include "board.h"
#include "command.h"
#include "esp_log.h"
#include "esp_peripherals.h"
#include "fatfs_stream.h"
#include "filter_resample.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "i2s_stream.h"
#include "input_key_service.h"
#include "library.h"
#include "mp3_decoder.h"
#include "nvs_flash.h"
#include "periph_adc_button.h"
#include "periph_button.h"
#include "periph_sdcard.h"
#include "periph_touch.h"

audio_board_handle_t board_handle;
audio_pipeline_handle_t pipeline;
audio_event_iface_handle_t evt;
audio_element_handle_t i2s_stream_writer, mp3_decoder, fatfs_stream_reader;
LibraryEntry *playing_entry = NULL;

void player_initialize() {
  ESP_LOGI(PLAYER_TAG, "[1.0] Initialize peripherals management");
  esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
  esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

  ESP_LOGI(PLAYER_TAG, "[1.1] Initialize and start peripherals");
  audio_board_key_init(set);
  audio_board_sdcard_init(set);

  ESP_LOGI(PLAYER_TAG, "[ 2 ] Start codec chip");
  board_handle = audio_board_init();
  audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

  ESP_LOGI(PLAYER_TAG, "[4.0] Create audio pipeline for playback");
  audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
  pipeline = audio_pipeline_init(&pipeline_cfg);
  mem_assert(pipeline);

  ESP_LOGI(PLAYER_TAG, "[4.1] Create i2s stream to write data to codec chip");
  i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
  i2s_cfg.i2s_config.sample_rate = 48000;
  i2s_cfg.type = AUDIO_STREAM_WRITER;
  i2s_stream_writer = i2s_stream_init(&i2s_cfg);

  ESP_LOGI(PLAYER_TAG, "[4.2] Create mp3 decoder to decode mp3 file");
  mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
  mp3_decoder = mp3_decoder_init(&mp3_cfg);

  ESP_LOGI(PLAYER_TAG, "[4.4] Create fatfs stream to read data from sdcard");
  fatfs_stream_cfg_t fatfs_cfg = FATFS_STREAM_CFG_DEFAULT();
  fatfs_cfg.type = AUDIO_STREAM_READER;
  fatfs_stream_reader = fatfs_stream_init(&fatfs_cfg);

  ESP_LOGI(PLAYER_TAG, "[4.5] Register all elements to audio pipeline");
  audio_pipeline_register(pipeline, fatfs_stream_reader, "file");
  audio_pipeline_register(pipeline, mp3_decoder, "mp3");
  audio_pipeline_register(pipeline, i2s_stream_writer, "i2s");

  ESP_LOGI(PLAYER_TAG, "[4.6] Link it together [sdcard]-->fatfs_stream-->mp3_decoder-->i2s_stream-->[codec_chip]");
  const char *link_tag[3] = {"file", "mp3", "i2s"};
  audio_pipeline_link(pipeline, &link_tag[0], 3);

  ESP_LOGI(PLAYER_TAG, "[5.0] Set up  event listener");
  audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
  evt = audio_event_iface_init(&evt_cfg);

  ESP_LOGI(PLAYER_TAG, "[5.1] Listen for all pipeline events");
  audio_pipeline_set_listener(pipeline, evt);
}

void start_playback(LibraryEntry *entry) {
  if (playing_entry != NULL && compare_uid(playing_entry->uid, entry->uid)) return;
  playing_entry = entry;
  ESP_LOGI(PLAYER_TAG, "Starting playback of %s", entry->name);

  audio_pipeline_stop(pipeline);
  audio_element_set_uri(fatfs_stream_reader, entry->file_path);
  audio_pipeline_reset_ringbuffer(pipeline);
  audio_pipeline_reset_elements(pipeline);
  audio_pipeline_change_state(pipeline, AEL_STATE_INIT);
  audio_pipeline_run(pipeline);
}

void stop_playback() {
  if (playing_entry != NULL) {
    ESP_LOGI(PLAYER_TAG, "Stopping playback of %s", playing_entry->name);
    audio_pipeline_stop(pipeline);
    playing_entry = NULL;
  }
}

void volume_up() {
  int player_volume;
  audio_hal_get_volume(board_handle->audio_hal, &player_volume);
  player_volume = MAX((player_volume + VOLUME_STEP), VOLUME_MAX);
  audio_hal_set_volume(board_handle->audio_hal, player_volume);
  ESP_LOGI(PLAYER_TAG, "Set Volume to %d", player_volume);
}

void volume_down() {
  int player_volume;
  audio_hal_get_volume(board_handle->audio_hal, &player_volume);
  player_volume = MAX((player_volume - VOLUME_STEP), 0);
  audio_hal_set_volume(board_handle->audio_hal, player_volume);
  ESP_LOGI(PLAYER_TAG, "Set Volume to %d", player_volume);
}

void pause_playback() {}

void player_task(void *pvParameters) {
  Command command;
  player_initialize();

  while (1) {
    /* Handle event interface messages from pipeline
       to set music info and to advance to the next song
    */
    audio_event_iface_msg_t msg;
    esp_err_t ret = audio_event_iface_listen(evt, &msg, 10);
    if (ret != ESP_OK) {
      if (xQueueReceive(command_queue, &command, 0) == pdTRUE) {
        switch (command.type) {
          case COMMAND_TYPE_PLAY:
            ESP_LOGI(PLAYER_TAG, "Received Play");
            start_playback(command.data);
            break;
          case COMMAND_TYPE_STOP:
            ESP_LOGI(PLAYER_TAG, "Received STOP");
            stop_playback();
            break;
          case COMMAND_TYPE_VOLUME_DOWN:
            ESP_LOGI(PLAYER_TAG, "Received Volume Down");
            volume_down();
            break;
          case COMMAND_TYPE_VOLUME_UP:
            ESP_LOGI(PLAYER_TAG, "Received Volume Up");
            volume_up();
            break;
          case COMMAND_TYPE_NEXT:
            ESP_LOGI(PLAYER_TAG, "Received Next");
            ESP_LOGW(PLAYER_TAG, "Not Implemented !");
            break;
          case COMMAND_TYPE_PREVIOUS:
            ESP_LOGI(PLAYER_TAG, "Received Previous");
            ESP_LOGW(PLAYER_TAG, "Not Implemented !");
            break;
          default:
            break;
        }
      }
      continue;
    }
    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) {
      // Set music info for a new song to be played
      if (msg.source == (void *)mp3_decoder && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
        audio_element_info_t music_info = {0};
        audio_element_getinfo(mp3_decoder, &music_info);
        music_info.
        ESP_LOGI(PLAYER_TAG, "[ * ] Received music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
                 music_info.sample_rates, music_info.bits, music_info.channels);
        audio_element_setinfo(i2s_stream_writer, &music_info);
        continue;
      }
      // clear playing file after playback
      if (msg.source == (void *)i2s_stream_writer && msg.cmd == AEL_MSG_CMD_REPORT_STATUS) {
        audio_element_state_t el_state = audio_element_get_state(i2s_stream_writer);
        if (el_state == AEL_STATE_FINISHED) {
          ESP_LOGI(PLAYER_TAG, "[ * ] Finished, wait for next command");
          playing_entry = NULL;
        }
        continue;
      }
    }
  }
}
