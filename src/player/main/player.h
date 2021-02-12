#if !defined(PLAYER_H)
#define PLAYER_H

#include <stdbool.h>

#define PLAYER_TAG "Player"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define VOLUME_MAX 30
#define VOLUME_STEP 5

void player_task(void *pvParameters);
void player_initialize();

#endif  // PLAYER_H
