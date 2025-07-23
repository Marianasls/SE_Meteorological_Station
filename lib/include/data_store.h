#ifndef DATA_STORE_H
#define DATA_STORE_H

#include <stdbool.h>
#include <stdint.h>
#include "pico/stdlib.h"

#define MAX_READINGS 30

// Estrutura para armazenar uma leitura dos sensores
typedef struct {
    float temperature;   // em °C
    float humidity;      // em %
    float pressure;      // em hPa
    uint32_t timestamp;  // em segundos desde o boot
} SensorReading;

// Estrutura para a pilha de leituras
typedef struct {
    SensorReading readings[MAX_READINGS];  // Array de leituras
    int count;                            // Número atual de leituras
} ReadingStore;

// Inicializa o armazenamento de leituras
void reading_store_init(ReadingStore* store);

// Adiciona uma nova leitura (e remove a mais antiga se necessário)
void reading_store_add(ReadingStore* store, float temp, float humidity, float pressure);

// Obter todas as leituras armazenadas
const SensorReading* reading_store_get_all(ReadingStore* store, int* count);

// Obter a última leitura
const SensorReading* reading_store_get_last(ReadingStore* store);

// Limpar todas as leituras
void reading_store_clear(ReadingStore* store);

#endif // DATA_STORE_H
