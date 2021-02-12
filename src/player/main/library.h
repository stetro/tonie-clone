#if !defined(LIBRARY_H)
#define LIBRARY_H

#include <stdbool.h>
#include <stdio.h>

typedef struct LibraryEntry {
  __uint8_t id;
  __uint8_t uid[4];
  const char* name;
  const char* file_path;
} LibraryEntry;

LibraryEntry* find_library_entry(__uint8_t* uid);

bool compare_uid(__uint8_t* one, __uint8_t* other);

extern LibraryEntry library_entries[];

#endif  // LIBRARY_H
