/**
 * @file sht3x_parser.h
 * @brief SHT3X Sensor Data Parser Library
 */
#ifndef SHT3X_PARSER_H
#define SHT3X_PARSER_H

/* INCLUDES ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* DEFINES -------------------------------------------------------------------*/
#define SHT3X_MODE_SINGLE       "SINGLE"
#define SHT3X_MODE_PERIODIC     "PERIODIC"

/* TYPEDEFS ------------------------------------------------------------------*/
typedef enum {
    SHT3X_TYPE_UNKNOWN = 0,
    SHT3X_TYPE_SINGLE,
    SHT3X_TYPE_PERIODIC
} sht3x_type_t;

typedef struct {
    sht3x_type_t type;
    float temperature; 
    float humidity;
    bool valid;
} sht3x_data_t;

typedef void (*sht3x_data_callback_t)(const sht3x_data_t* data);

typedef struct {
    sht3x_data_callback_t single_callback;
    sht3x_data_callback_t periodic_callback;
} sht3x_parser_t;

/* GLOBAL FUNCTIONS ----------------------------------------------------------*/

/**
 * @brief Initialize sht3x parser
 * 
 * @param parser Sht3x parser structure
 * @param single_callback Callback for single measurements
 * @param periodic_callback Callback for periodic measurements
 * 
 * @return true if successful
 */
bool SHT3X_Parser_Init(sht3x_parser_t *parser, 
                       sht3x_data_callback_t single_callback,
                       sht3x_data_callback_t periodic_callback);

/**
 * @brief Parse sht3x data line
 * 
 * @param parser Sht3x parser structure
 * @param line Data line from STM32 (e.g., "SINGLE 27.85 85.69")
 * 
 * @return Parsed sht3x data structure
 */
sht3x_data_t SHT3X_Parser_ParseLine(sht3x_parser_t *parser, const char* line);

/**
 * @brief Process sht3x data line with callbacks
 * 
 * @param parser Sht3x parser structure
 * @param line Data line from STM32
 * 
 * @return true if line was successfully parsed and processed
 */
bool SHT3X_Parser_ProcessLine(sht3x_parser_t *parser, const char* line);

/**
 * @brief Get sht3x type from string
 * 
 * @param type_str Type string ("SINGLE" or "PERIODIC")
 * 
 * @return Sht3x type enum
 */
sht3x_type_t SHT3X_Parser_GetType(const char* type_str);

/**
 * @brief Get sht3x type string
 * 
 * @param type Sht3x type enum
 * 
 * @return Type string
 */
const char* SHT3X_Parser_GetTypeString(sht3x_type_t type);

/**
 * @brief Validate sht3x data
 * 
 * @param data Sht3x data structure
 * 
 * @return true if data is valid
 */
bool SHT3X_Parser_IsValid(const sht3x_data_t* data);

#endif /* SHT3X_PARSER_H */