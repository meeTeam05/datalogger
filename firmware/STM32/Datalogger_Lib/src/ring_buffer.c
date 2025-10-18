/**
 * @file ring_buffer.c
 *
 * @brief Ring Buffer Source
 */

/* INCLUDES ------------------------------------------------------------------*/

#include "ring_buffer.h"

/* PUBLIC API ----------------------------------------------------------------*/

void RingBuffer_Init(ring_buffer_t *rb)
{
	rb->head = 0;
	rb->tail = 0;
}

bool RingBuffer_Put(ring_buffer_t *rb, uint8_t data)
{
	uint16_t next = (rb->head + 1) % RING_BUFFER_SIZE;
	if (next == rb->tail)
	{
		// buffer full
		return false;
	}
	rb->buffer[rb->head] = data;
	rb->head = next;
	return true;
}

bool RingBuffer_Get(ring_buffer_t *rb, uint8_t *data)
{
	if (rb->head == rb->tail)
	{
		// buffer empty
		return false;
	}
	*data = rb->buffer[rb->tail];
	rb->tail = (rb->tail + 1) % RING_BUFFER_SIZE;
	return true;
}

uint16_t RingBuffer_Available(ring_buffer_t *rb)
{
	if (rb->head >= rb->tail)
	{
		return rb->head - rb->tail;
	}
	else
	{
		return RING_BUFFER_SIZE - (rb->tail - rb->head);
	}
}

void RingBuffer_Clear(ring_buffer_t *rb)
{
	if (rb)
	{
		rb->head = 0;
		rb->tail = 0;
	}
}

uint16_t RingBuffer_Free(ring_buffer_t *rb)
{
	return RING_BUFFER_SIZE - RingBuffer_Available(rb) - 1;
}
