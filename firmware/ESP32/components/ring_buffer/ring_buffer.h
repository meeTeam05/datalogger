/**
 * @file ring_buffer.h
 */
#ifndef RING_BUFFER_H
#define RING_BUFFER_H

/* INCLUDES ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* DEFINES -------------------------------------------------------------------*/
#define RING_BUFFER_SIZE 256

/* TYPEDEFS ------------------------------------------------------------------*/

/*
 * @brief Ring buffer structure
 */
typedef struct {
	uint8_t buffer[RING_BUFFER_SIZE];
	volatile uint16_t head;
	volatile uint16_t tail;
} ring_buffer_t;

/* GLOBAL FUNCTIONS ----------------------------------------------------------*/

/**
 * @brief Initialize ring buffer
 *
 * @param *rb Pointer to ring buffer structure
 */
void RingBuffer_Init(ring_buffer_t *rb);

/**
 * @brief Put data into ring buffer
 *
 * @param *rb Pointer to ring buffer structure
 * @param data Data byte to put
 *
 * @return true if successful, false if buffer full
 */
bool RingBuffer_Put(ring_buffer_t *rb, uint8_t data);

/**
 * @brief Get data from ring buffer
 *
 * @param *rb Pointer to ring buffer structure
 * @param *data Pointer to store retrieved data
 *
 * @return true if successful, false if buffer empty
 */
bool RingBuffer_Get(ring_buffer_t *rb, uint8_t *data);

/**
 * @brief Get number of available bytes in ring buffer
 *
 * @param *rb Pointer to ring buffer structure
 *
 * @return Number of available bytes
 */
uint16_t RingBuffer_Available(ring_buffer_t *rb);

/**
 * @brief Clear all data in ring buffer
 * 
 * @param rb Pointer to ring buffer structure
 */
void RingBuffer_Clear(ring_buffer_t *rb);

/**
 * @brief Get number of free bytes in ring buffer
 *
 * @param *rb Pointer to ring buffer structure
 *
 * @return Number of free bytes
 */
uint16_t RingBuffer_Free(ring_buffer_t *rb);

#endif /* RING_BUFFER_H */