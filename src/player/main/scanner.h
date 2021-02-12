#if !defined(SCANNER_H)
#define SCANNER_H

#define SCANNER_TAG "Scanner"
#define SCANNER_INTERVAL 1000
#include <stdbool.h>

void scanner_task(void *pvParameters);

bool scanner_initialize();

#endif  // SCANNER_H
