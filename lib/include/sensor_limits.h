// ============================================================================
// ESTRUTURA PARA ARMAZENAR LIMITES DOS SENSORES
// ============================================================================

#ifndef SENSOR_LIMITS_H
#define SENSOR_LIMITS_H

#include <stdbool.h>

// Estrutura para armazenar os limites de cada sensor
typedef struct {
    // Limites de temperatura (°C)
    float min_temp;
    float max_temp;
    
    // Limites de umidade (%)
    float min_humidity;
    float max_humidity;
    
    // Limites de pressão atmosférica (hPa)
    float min_pressure;
    float max_pressure;
    
    // Limites de altitude (metros) - calculada a partir da pressão
    float min_altitude;
    float max_altitude;
    
    // Flags para indicar se os limites foram configurados
    bool limits_configured;
    bool alert_enabled;
} SensorLimits;

// Variável global para os limites
extern SensorLimits sensor_limits;

// ============================================================================
// FUNÇÕES PARA GERENCIAR OS LIMITES
// ============================================================================

/**
 * Inicializa os limites com valores padrão
 */
void sensor_limits_init(SensorLimits* limits) {
    // Limites padrão baseados em condições normais
    limits->min_temp = 15.0f;        // 15°C mínimo
    limits->max_temp = 35.0f;        // 35°C máximo
    
    limits->min_humidity = 30.0f;    // 30% umidade mínima
    limits->max_humidity = 80.0f;    // 80% umidade máxima
    
    limits->min_pressure = 980.0f;   // 980 hPa pressão mínima
    limits->max_pressure = 1030.0f;  // 1030 hPa pressão máxima
    
    limits->min_altitude = -100.0f;  // -100m altitude mínima
    limits->max_altitude = 1000.0f;  // 1000m altitude máxima
    
    limits->limits_configured = true;
    limits->alert_enabled = true;
}

/**
 * Define novos limites de temperatura
 */
void sensor_limits_set_temperature(SensorLimits* limits, float min_temp, float max_temp) {
    if (min_temp < max_temp) {
        limits->min_temp = min_temp;
        limits->max_temp = max_temp;
        printf("Limites de temperatura atualizados: %.1f°C - %.1f°C\n", min_temp, max_temp);
    } else {
        printf("ERRO: Temperatura mínima deve ser menor que máxima!\n");
    }
}

/**
 * Define novos limites de umidade
 */
void sensor_limits_set_humidity(SensorLimits* limits, float min_hum, float max_hum) {
    if (min_hum >= 0 && max_hum <= 100 && min_hum < max_hum) {
        limits->min_humidity = min_hum;
        limits->max_humidity = max_hum;
        printf("Limites de umidade atualizados: %.1f%% - %.1f%%\n", min_hum, max_hum);
    } else {
        printf("ERRO: Limites de umidade inválidos (0-100%% e min < max)!\n");
    }
}

/**
 * Define novos limites de pressão
 */
void sensor_limits_set_pressure(SensorLimits* limits, float min_press, float max_press) {
    if (min_press > 0 && max_press > 0 && min_press < max_press) {
        limits->min_pressure = min_press;
        limits->max_pressure = max_press;
        printf("Limites de pressão atualizados: %.1f hPa - %.1f hPa\n", min_press, max_press);
    } else {
        printf("ERRO: Limites de pressão inválidos!\n");
    }
}

/**
 * Define todos os limites de uma vez
 */
void sensor_limits_set_all(SensorLimits* limits, 
                          float min_temp, float max_temp,
                          float min_hum, float max_hum,
                          float min_press, float max_press) {
    sensor_limits_set_temperature(limits, min_temp, max_temp);
    sensor_limits_set_humidity(limits, min_hum, max_hum);
    sensor_limits_set_pressure(limits, min_press, max_press);
    
    limits->limits_configured = true;
    printf("Todos os limites foram configurados!\n");
}

// ============================================================================
// FUNÇÕES DE VERIFICAÇÃO DE ALERTAS
// ============================================================================

/**
 * Verifica se a temperatura está dentro dos limites
 */
bool sensor_limits_check_temperature(const SensorLimits* limits, float temperature) {
    if (!limits->alert_enabled) return true;
    return (temperature >= limits->min_temp && temperature <= limits->max_temp);
}

/**
 * Verifica se a umidade está dentro dos limites
 */
bool sensor_limits_check_humidity(const SensorLimits* limits, float humidity) {
    if (!limits->alert_enabled) return true;
    return (humidity >= limits->min_humidity && humidity <= limits->max_humidity);
}

/**
 * Verifica se a pressão está dentro dos limites
 */
bool sensor_limits_check_pressure(const SensorLimits* limits, float pressure) {
    if (!limits->alert_enabled) return true;
    return (pressure >= limits->min_pressure && pressure <= limits->max_pressure);
}

/**
 * Estrutura para resultado da verificação de limites
 */
typedef struct {
    bool temperature_ok;
    bool humidity_ok;
    bool pressure_ok;
    bool all_ok;
    char alert_message[256];
} LimitCheckResult;

/**
 * Verifica todos os sensores e retorna resultado detalhado
 */
LimitCheckResult sensor_limits_check_all(const SensorLimits* limits, 
                                        float temperature, 
                                        float humidity, 
                                        float pressure) {
    LimitCheckResult result = {0};
    char temp_msg[64] = "";
    char hum_msg[64] = "";
    char press_msg[64] = "";
    
    // Verifica temperatura
    result.temperature_ok = sensor_limits_check_temperature(limits, temperature);
    if (!result.temperature_ok) {
        if (temperature < limits->min_temp) {
            snprintf(temp_msg, sizeof(temp_msg), "Temp BAIXA: %.1f°C (min: %.1f°C) ", 
                    temperature, limits->min_temp);
        } else {
            snprintf(temp_msg, sizeof(temp_msg), "Temp ALTA: %.1f°C (max: %.1f°C) ", 
                    temperature, limits->max_temp);
        }
    }
    
    // Verifica umidade
    result.humidity_ok = sensor_limits_check_humidity(limits, humidity);
    if (!result.humidity_ok) {
        if (humidity < limits->min_humidity) {
            snprintf(hum_msg, sizeof(hum_msg), "Umid BAIXA: %.1f%% (min: %.1f%%) ", 
                    humidity, limits->min_humidity);
        } else {
            snprintf(hum_msg, sizeof(hum_msg), "Umid ALTA: %.1f%% (max: %.1f%%) ", 
                    humidity, limits->max_humidity);
        }
    }
    
    // Verifica pressão
    result.pressure_ok = sensor_limits_check_pressure(limits, pressure);
    if (!result.pressure_ok) {
        if (pressure < limits->min_pressure) {
            snprintf(press_msg, sizeof(press_msg), "Press BAIXA: %.1f hPa (min: %.1f hPa)", 
                    pressure, limits->min_pressure);
        } else {
            snprintf(press_msg, sizeof(press_msg), "Press ALTA: %.1f hPa (max: %.1f hPa)", 
                    pressure, limits->max_pressure);
        }
    }
    
    // Resultado geral
    result.all_ok = result.temperature_ok && result.humidity_ok && result.pressure_ok;
    
    // Monta mensagem de alerta
    if (result.all_ok) {
        snprintf(result.alert_message, sizeof(result.alert_message), "Todos os sensores OK");
    } else {
        snprintf(result.alert_message, sizeof(result.alert_message), "ALERTA: %s%s%s", 
                temp_msg, hum_msg, press_msg);
    }
    
    return result;
}

// ============================================================================
// FUNÇÕES PARA PERSISTÊNCIA (SIMULADA EM MEMÓRIA)
// ============================================================================

/**
 * Salva os limites em uma estrutura de configuração
 * Em um sistema real, isso seria salvo em EEPROM/Flash
 */
typedef struct {
    SensorLimits limits;
    uint32_t checksum;  // Para verificar integridade
    bool valid;
} StoredLimits;

static StoredLimits stored_limits = {0};

/**
 * Calcula checksum simples para verificar integridade
 */
uint32_t calculate_limits_checksum(const SensorLimits* limits) {
    uint32_t checksum = 0;
    const uint8_t* data = (const uint8_t*)limits;
    for (size_t i = 0; i < sizeof(SensorLimits); i++) {
        checksum += data[i];
    }
    return checksum;
}

/**
 * Salva os limites na memória (simulando persistência)
 */
bool sensor_limits_save(const SensorLimits* limits) {
    stored_limits.limits = *limits;
    stored_limits.checksum = calculate_limits_checksum(limits);
    stored_limits.valid = true;
    
    printf("Limites salvos com sucesso!\n");
    return true;
}

/**
 * Carrega os limites da memória
 */
bool sensor_limits_load(SensorLimits* limits) {
    if (!stored_limits.valid) {
        printf("Nenhum limite salvo encontrado. Usando padrões.\n");
        return false;
    }
    
    // Verifica integridade
    uint32_t current_checksum = calculate_limits_checksum(&stored_limits.limits);
    if (current_checksum != stored_limits.checksum) {
        printf("ERRO: Dados corrompidos! Usando limites padrão.\n");
        return false;
    }
    
    *limits = stored_limits.limits;
    printf("Limites carregados com sucesso!\n");
    return true;
}

/**
 * Imprime os limites atuais de forma formatada
 */
void sensor_limits_print(const SensorLimits* limits) {
    printf("\n=== LIMITES DOS SENSORES ===\n");
    printf("Temperatura: %.1f°C - %.1f°C\n", limits->min_temp, limits->max_temp);
    printf("Umidade:     %.1f%% - %.1f%%\n", limits->min_humidity, limits->max_humidity);
    printf("Pressão:     %.1f hPa - %.1f hPa\n", limits->min_pressure, limits->max_pressure);
    printf("Altitude:    %.1f m - %.1f m\n", limits->min_altitude, limits->max_altitude);
    printf("Alertas:     %s\n", limits->alert_enabled ? "ATIVADOS" : "DESATIVADOS");
    printf("============================\n\n");
}

#endif // SENSOR_LIMITS_H
