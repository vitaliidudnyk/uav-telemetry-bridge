#include "ring_buffer.h"

static size_t min_sz(size_t a, size_t b)
{
  return (a < b) ? a : b;
}

bool RingBuffer_Init(RingBuffer* rb, uint8_t* storage, size_t capacity)
{
  if (rb == nullptr || storage == nullptr || capacity == 0)
  {
    return false;
  }

  rb->data = storage;
  rb->capacity = capacity;
  rb->head = 0;
  rb->tail = 0;
  rb->size = 0;
  return true;
}

size_t RingBuffer_Capacity(const RingBuffer* rb)
{
  return (rb != nullptr) ? rb->capacity : 0;
}

size_t RingBuffer_Size(const RingBuffer* rb)
{
  return (rb != nullptr) ? rb->size : 0;
}

size_t RingBuffer_Free(const RingBuffer* rb)
{
  if (rb == nullptr)
  {
    return 0;
  }
  return rb->capacity - rb->size;
}

size_t RingBuffer_Write(RingBuffer* rb, const uint8_t* src, size_t len)
{
  if (rb == nullptr || src == nullptr || len == 0)
  {
    return 0;
  }

  size_t to_write = min_sz(len, RingBuffer_Free(rb));
  if (to_write == 0)
  {
    return 0;
  }

  size_t first = min_sz(to_write, rb->capacity - rb->head);
  for (size_t i = 0; i < first; i++)
  {
    rb->data[rb->head + i] = src[i];
  }

  size_t second = to_write - first;
  for (size_t i = 0; i < second; i++)
  {
    rb->data[i] = src[first + i];
  }

  rb->head = (rb->head + to_write) % rb->capacity;
  rb->size += to_write;
  return to_write;
}

size_t RingBuffer_Read(RingBuffer* rb, uint8_t* dst, size_t len)
{
  if (rb == nullptr || dst == nullptr || len == 0)
  {
    return 0;
  }

  size_t to_read = min_sz(len, rb->size);
  if (to_read == 0)
  {
    return 0;
  }

  size_t first = min_sz(to_read, rb->capacity - rb->tail);
  for (size_t i = 0; i < first; i++)
  {
    dst[i] = rb->data[rb->tail + i];
  }

  size_t second = to_read - first;
  for (size_t i = 0; i < second; i++)
  {
    dst[first + i] = rb->data[i];
  }

  rb->tail = (rb->tail + to_read) % rb->capacity;
  rb->size -= to_read;
  return to_read;
}

size_t RingBuffer_Peek(const RingBuffer* rb, uint8_t* dst, size_t len)
{
  if (rb == nullptr || dst == nullptr || len == 0)
  {
    return 0;
  }

  size_t to_peek = min_sz(len, rb->size);
  if (to_peek == 0)
  {
    return 0;
  }

  size_t first = min_sz(to_peek, rb->capacity - rb->tail);
  for (size_t i = 0; i < first; i++)
  {
    dst[i] = rb->data[rb->tail + i];
  }

  size_t second = to_peek - first;
  for (size_t i = 0; i < second; i++)
  {
    dst[first + i] = rb->data[i];
  }

  return to_peek;
}

size_t RingBuffer_Skip(RingBuffer* rb, size_t len)
{
  if (rb == nullptr || len == 0)
  {
    return 0;
  }

  size_t to_skip = min_sz(len, rb->size);
  if (to_skip == 0)
  {
    return 0;
  }

  rb->tail = (rb->tail + to_skip) % rb->capacity;
  rb->size -= to_skip;
  return to_skip;
}

void RingBuffer_Clear(RingBuffer* rb)
{
  if (rb == nullptr)
  {
    return;
  }

  rb->head = 0;
  rb->tail = 0;
  rb->size = 0;
}
