#ifndef __SDH_ARENA
#define __SDH_ARENA

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
#define Terabytes(Value) (Gigabytes(Value) * 1024LL)

typedef struct {
  size_t size;
  uint8_t *base;
  size_t used;
  size_t tmp_count;
} MemoryArena;

typedef struct {
  MemoryArena *arena;
  size_t used;
} TemporaryMemory;

void initializeArena(MemoryArena *arena, size_t size, void *base) {
  arena->size = size;
  arena->base = (uint8_t *)base;
  arena->used = 0;
}

inline static size_t getAlignmentOffset(MemoryArena *arena, size_t alignment) {
  size_t resultPointer = (size_t)arena->base + arena->used;
  size_t alignmentOffset = 0;

  size_t alignmentMask = alignment - 1;
  if (resultPointer & alignmentMask) {
    alignmentOffset = alignment - (resultPointer & alignmentMask);
  }

  return alignmentOffset;
}

#define DEFAULT_ALIGNMENT 4
void *push_size(MemoryArena *arena, size_t size, size_t alignment) {
  size_t originalSize = size;
  size_t alignmentOffset = getAlignmentOffset(arena, alignment);
  size += alignmentOffset;

  assert((arena->used + size) <= arena->size);
  void *result = arena->base + arena->used + alignmentOffset;
  arena->used += size;

  assert(size >= originalSize);

  return result;
}

char *push_string(MemoryArena *arena, char *source) {
  uint32_t size = strlen(source) + 1;
  char *dest = (char *)push_size(arena, size, DEFAULT_ALIGNMENT);
  for (uint32_t charIndex = 0; charIndex < size; ++charIndex) {
    dest[charIndex] = source[charIndex];
  }
  return dest;
}

TemporaryMemory begin_temporary_memory(MemoryArena *arena) {
  TemporaryMemory result;
  result.arena = arena;
  result.used = arena->used;
  ++arena->tmp_count;
  return result;
}

void end_temporary_memory(TemporaryMemory temporaryMemory) {
  MemoryArena *arena = temporaryMemory.arena;
  assert(arena->used >= temporaryMemory.used);
  arena->used = temporaryMemory.used;
  assert(arena->tmp_count > 0);
  arena->tmp_count--;
}

#endif
