#if !defined(COMMAND_H)
#define COMMAND_H

#include "freertos/queue.h"
#include "library.h"

#define COMMAND_DATA_LENGTH 20

#define COMMAND_TYPE_PLAY 1
#define COMMAND_TYPE_STOP 2
#define COMMAND_TYPE_VOLUME_UP 3
#define COMMAND_TYPE_VOLUME_DOWN 4
#define COMMAND_TYPE_NEXT 5
#define COMMAND_TYPE_PREVIOUS 6


typedef struct Command {
  int type;
  LibraryEntry *data;
} Command;

QueueHandle_t command_queue;

#endif  // COMMAND_H
