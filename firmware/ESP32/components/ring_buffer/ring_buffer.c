/**
 * @file ring_buffer.c
 */

/* INCLUDES ------------------------------------------------------------------*/

#include "ring_buffer.h"

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Initialize ring buffer
 */
void RingBuffer_Init(ring_buffer_t *rb)
{
	rb->head = 0;
	rb->tail = 0;
}

/**
 * @brief Put data into ring buffer
 */
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

/**
 * @brief Get data from ring buffer
 */
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

/**
 * @brief Get number of available bytes in ring buffer
 */
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

/**
 * @brief Clear all data in ring buffer
 */
void RingBuffer_Clear(ring_buffer_t *rb)
{
	if (rb)
	{
		rb->head = 0;
		rb->tail = 0;
	}
}

/**
 * @brief Get number of free bytes in ring buffer
 */
uint16_t RingBuffer_Free(ring_buffer_t *rb)
{
	return RING_BUFFER_SIZE - RingBuffer_Available(rb) - 1;
}