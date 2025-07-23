#include "data_store.h"
#include <string.h>

void reading_store_init(ReadingStore* store) {
    // Inicializa a estrutura de armazenamento com valores padrão
    if (store) {
        memset(store, 0, sizeof(ReadingStore));
    }
}

void reading_store_add(ReadingStore* store, float temp, float humidity, float pressure) {
    if (!store) return;
    
    // Se a pilha estiver cheia, desloque todos os elementos para baixo (remova o último)
    if (store->count == MAX_READINGS) {
        // Desloca todos os elementos uma posição para baixo
        for (int i = 0; i < MAX_READINGS - 1; i++) {
            store->readings[i] = store->readings[i + 1];
        }
        // O contador permanece o mesmo (MAX_READINGS)
    } else {
        // Ainda tem espaço, incrementa o contador
        store->count++;
    }
    
    // Adiciona a nova leitura no topo (último índice usado)
    SensorReading* new_reading = &store->readings[store->count - 1];
    new_reading->temperature = temp;
    new_reading->humidity = humidity;
    new_reading->pressure = pressure;
    new_reading->timestamp = to_ms_since_boot(get_absolute_time()) / 1000; // segundos desde o boot
}

const SensorReading* reading_store_get_all(ReadingStore* store, int* count) {
    if (store && count) {
        *count = store->count;
        return store->readings;
    }
    if (count) *count = 0;
    return NULL;
}

const SensorReading* reading_store_get_last(ReadingStore* store) {
    if (store && store->count > 0) {
        return &store->readings[store->count - 1];
    }
    return NULL;
}

void reading_store_clear(ReadingStore* store) {
    if (store) {
        store->count = 0;
    }
}
