#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct RingBuffer
{
  uint8_t* data;
  size_t capacity;   // total bytes
  size_t head;       // write index
  size_t tail;       // read index
  size_t size;       // used bytes
} RingBuffer;

// Initializes ring buffer over externally provided memory.
// Returns false if params invalid.
bool RingBuffer_Init(RingBuffer* rb, uint8_t* storage, size_t capacity);

size_t RingBuffer_Capacity(const RingBuffer* rb);
size_t RingBuffer_Size(const RingBuffer* rb);
size_t RingBuffer_Free(const RingBuffer* rb);

// Writes up to len bytes. Returns how many bytes were actually written.
size_t RingBuffer_Write(RingBuffer* rb, const uint8_t* src, size_t len);

// Reads up to len bytes. Returns how many bytes were actually read.
size_t RingBuffer_Read(RingBuffer* rb, uint8_t* dst, size_t len);

// Peeks up to len bytes into dst without removing them. Returns bytes copied.
size_t RingBuffer_Peek(const RingBuffer* rb, uint8_t* dst, size_t len);

// Advances tail by len bytes (used after Peek+Send). Returns bytes actually skipped.
size_t RingBuffer_Skip(RingBuffer* rb, size_t len);

// Clears buffer.
void RingBuffer_Clear(RingBuffer* rb);
