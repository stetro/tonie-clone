#include "library.h"

#include <stdbool.h>
#include <stdio.h>

LibraryEntry library_entries[] = {
    {.id = 0, .uid = {0x7B, 0xC9, 0xF4, 0x0E}, .name = "Leo Lausemaus", .file_path = "/sdcard/leo.mp3"},
    {.id = 1, .uid = {0x4B, 0x64, 0x8F, 0x1B}, .name = "Sendung mit der Maus", .file_path = "/sdcard/maus.mp3"}};

bool compare_uid(__uint8_t* one, __uint8_t* other) {
  return one[0] == other[0] && one[1] == other[1] && one[2] == other[2] && one[3] == other[3];
}

LibraryEntry* find_library_entry(__uint8_t* uid) {
  for (int i = 0; i < (sizeof(library_entries) / sizeof(LibraryEntry)); i++) {
    if (compare_uid(uid, library_entries[i].uid)) {
      return library_entries + i;
    }
  }
  return NULL;
}
