#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"

#include "ssd1306.h"
#include "font.h"
#include "bmp280.h"
#include "aht20.h"
#include "lwipopts.h"
#include "globals.h"
#include "wifi.h"
#include "server.h"
#include "data_store.h"
#include "ws2812.h"
#include "ws2812.pio.h"
#include "buzzer.h"
#include "sensor_limits.h"
#include "page_html.h"

// Trecho para modo BOOTSEL com botão B
#include "pico/bootrom.h"
#define botaoB 6
void gpio_irq_handler(uint gpio, uint32_t events) {
    reset_usb_boot(0, 0);
}

// variaveis globais
struct bmp280_calib_param params;       // Estrutura de calibração do BMP280
ssd1306_t ssd;                        // Inicializa a estrutura do display
ReadingStore sensor_readings;           // Estrutura de pilha para armazenar as leituras dos sensores
SensorLimits sensor_limits;

// declaração de funções
void gpio_init_all();
void update_display();
void configureWiFi();
double calculate_altitude(double pressure);
static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t http_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err);
void start_http_server(void);
bool extract_json_float(const char* json_str, const char* key, float* value);

// Estrutura HTTP
struct http_state {
    char response[8000];
    size_t len;
    size_t sent;
};

int main() {
    stdio_init_all();
        
    gpio_init_all();

    // Inicializa o wifi
    configureWiFi();

    // Inicializa os limites dos sensores
    sensor_limits_init(&sensor_limits);

    // Tenta carregar limites salvos, se não conseguir usa os padrão
    if (!sensor_limits_load(&sensor_limits)) {
        printf("Usando limites padrão...\n");
        sensor_limits_init(&sensor_limits);
    }
    sensor_limits_print(&sensor_limits);

    // Inicializa o servidor HTTP
    printf("Iniciando servidor HTTP...\n");
    start_http_server();

    // Estrutura para armazenar os dados do sensor
    AHT20_Data data;
    int32_t raw_temp_bmp;
    int32_t raw_pressure;

    char str_tmp[8];  // Buffer maior para armazenar a string
    char str_press[8];  // Buffer maior para armazenar a string  
    char str_umi[8];  // Buffer maior para armazenar a string 

    bool cor = true;

    // Inicialize o armazenamento de leituras
    reading_store_init(&sensor_readings);
        
    while (1) {        
        // Leitura do BMP280
        bmp280_read_raw(I2C_PORT, &raw_temp_bmp, &raw_pressure);
        int32_t temperature = bmp280_convert_temp(raw_temp_bmp, &params);
        int32_t pressure = bmp280_convert_pressure(raw_pressure, raw_temp_bmp, &params);

        printf("Pressao = %.3f kPa\n", pressure / 1000.0);
        printf("Temperatura BMP: = %.2f C\n", temperature / 100.0);

        // Leitura do AHT20
        if (aht20_read(I2C_PORT, &data)){
            printf("Temperatura AHT: %.2f C\n", data.temperature);
            printf("Umidade: %.2f %%\n", data.humidity);
        } else {
            printf("Erro na leitura do AHT20!\n");
            // Use valores padrão em caso de erro
            data.temperature = 0.0f;
            data.humidity = 00.0f;
        }   
        
        // Calcule a média da temperatura entre os dois sensores
        float avg_temp = (temperature / 100.0f + data.temperature) / 2.0f;
        printf("Temperatura média: %.2f C\n", avg_temp);

        // Verifica limites ANTES de armazenar
        LimitCheckResult check_result = sensor_limits_check_all(&sensor_limits, 
                                                       avg_temp, 
                                                       data.humidity, 
                                                       pressure / 100.0f);
        
        if (!check_result.all_ok) {
            printf("⚠️  ALERTA: %s\n", check_result.alert_message);
            
            set_leds(255, 0, 0); // Vermelho
            
            cor = false; 
        } else {
            printf("✅ Todos os sensores OK\n");
            set_leds(0, 50, 0); // Verde suave
            cor = true; 
        }                                               
        
        sprintf(str_tmp, "%.1fC", avg_temp);  // Temperatura já em °C
        sprintf(str_umi, "%.1f%%", data.humidity);  // Umidade
        sprintf(str_press, "%.1f", pressure / 1000.0); // Pressão em kPa

        // Atualiza o conteúdo do display
        ssd1306_fill(&ssd, !cor);                           // Limpa o display
        ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);       // Desenha um retângulo
        ssd1306_line(&ssd, 3, 25, 123, 25, cor);            // Desenha uma linha
        ssd1306_line(&ssd, 3, 37, 123, 37, cor);            // Desenha uma linha
        ssd1306_draw_string(&ssd, "Estacao", 8, 8);         // Desenha uma string
        ssd1306_draw_string(&ssd, "Meteorologica", 8, 16);  // Desenha uma string
        ssd1306_draw_string(&ssd, "BMP280  AHT20", 10, 28); // Desenha uma string
        ssd1306_line(&ssd, 63, 25, 63, 60, cor);            // Desenha uma linha vertical
        ssd1306_draw_string(&ssd, str_tmp, 14, 41);         // Temperatura
        ssd1306_draw_string(&ssd, str_press, 14, 52);       // Pressão
        ssd1306_draw_string(&ssd, str_umi, 73, 45);         // Umidade
        ssd1306_send_data(&ssd);                            // Atualiza o display
        
        // Armazene a leitura na pilha (pressão em hPa para consistência)
        reading_store_add(&sensor_readings, avg_temp, data.humidity, pressure / 100.0f);

        cyw43_arch_poll(); // Necessário para manter o Wi-Fi ativo
        sleep_ms(1500); // Aumentado o delay para dar mais tempo ao sistema
    }
    
    cyw43_arch_deinit(); // Desliga o Wi-Fi antes de sair
    return 0;
}

void gpio_init_all() {
    printf("Inicializando GPIO...\n");
    
    // Para ser utilizado o modo BOOTSEL com botão B
    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // I2C do Display funcionando em 400Khz.
    printf("Inicializando display...\n");
    i2c_init(I2C_PORT_DISP, 400 * 1000);
    gpio_set_function(I2C_SDA_DISP, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_DISP, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_DISP);
    gpio_pull_up(I2C_SCL_DISP);
    
    ssd1306_init(&ssd, DISPLAY_WIDTH, DISPLAY_HEIGHT, false, DISPLAY_ADDRESS, I2C_PORT_DISP);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);

    // Limpa o display
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // Inicializa o I2C dos sensores BMP280 e AHT20
    printf("Inicializando sensores...\n");
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o BMP280
    bmp280_init(I2C_PORT);
    bmp280_get_calib_params(I2C_PORT, &params);

    // Inicializa o AHT20
    aht20_reset(I2C_PORT);
    aht20_init(I2C_PORT);

    // Inicializar a matriz de LEDs (WS2812)
    printf("Inicializando LEDs...\n");
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, false);

    clear_buffer();
    set_leds(0, 0, 0); // Limpa a matriz de LEDs
    
    printf("GPIO inicializado com sucesso!\n");
}

// Função para calcular a altitude a partir da pressão atmosférica
double calculate_altitude(double pressure) {
    return 44330.0 * (1.0 - pow(pressure / SEA_LEVEL_PRESSURE, 0.1903));
}

void configureWiFi() {
    // Inicialização do Wi-Fi
    printf("Inicializando Wi-Fi...\n");
    while(cyw43_arch_init()) {
        printf("ERRO: Falha ao inicializar o Wi-Fi! Tentando novamente em 5s...\n");
        sleep_ms(5000);
    }

    cyw43_arch_gpio_put(LED_PIN, 0);    // LED apagado inicialmente
    cyw43_arch_enable_sta_mode();       // Modo Station
    
    wifi_connected = false;
    printf("Conectando à rede: %s\n", WIFI_SSID);
    while(cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("ERRO: Falha ao conectar ao Wi-Fi. Tentando novamente...\n");
        sleep_ms(5000);
    }
    wifi_connected = true;
    printf("Conectado com sucesso!\n");
    snprintf(ip_address_str, sizeof(ip_address_str), "%s", ip4addr_ntoa(netif_ip4_addr(netif_default)));
    printf("Endereço IP: %s\n", ip_address_str);
    printf("Tamanho do HTML_BODY: %zu bytes\n", strlen(HTML_BODY));
}

// Função auxiliar para extrair valores JSON de uma string POST
bool extract_json_float(const char* json_str, const char* key, float* value) {
    char search_key[64];
    snprintf(search_key, sizeof(search_key), "\"%s\":", key);
    
    char* pos = strstr(json_str, search_key);
    if (pos) {
        pos += strlen(search_key);
        // Pula espaços em branco
        while (*pos == ' ' || *pos == '\t') pos++;
        *value = strtof(pos, NULL);
        return true;
    }
    return false;
}

static err_t http_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    struct http_state *hs = (struct http_state *)arg;
    printf("tcp_sent: %d bytes enviados\n", len);
    
    if (!hs) return ERR_OK;
    
    hs->sent += len;
    
    // Verifica se ainda há dados para enviar
    if (hs->sent < hs->len) {
        // Envia o restante dos dados
        size_t remaining = hs->len - hs->sent;
        size_t chunk_size = remaining > 1400 ? 1400 : remaining; // Valor seguro menor que TCP_MSS
        
        err_t err = tcp_write(tpcb, hs->response + hs->sent, chunk_size, TCP_WRITE_FLAG_COPY);
        if (err != ERR_OK) {
            printf("ERRO em http_sent: falha ao escrever dados TCP: %d\n", err);
            free(hs);
            tcp_arg(tpcb, NULL);
            tcp_sent(tpcb, NULL);
            tcp_close(tpcb);
            return err;
        }
        
        err = tcp_output(tpcb);
        if (err != ERR_OK) {
            printf("ERRO em http_sent: falha ao enviar dados TCP: %d\n", err);
        }
        
        return ERR_OK;
    } else {
        // Todos os dados foram enviados
        free(hs);
        tcp_arg(tpcb, NULL);
        tcp_sent(tpcb, NULL);
        tcp_close(tpcb);
        return ERR_OK;
    }
}

static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    char *req = (char *)p->payload;
    printf("HTTP Request recebida: %.50s...\n", req);
    
    struct http_state *hs = malloc(sizeof(struct http_state));
    if (!hs) {
        printf("ERRO: Falha ao alocar memória para http_state\n");
        pbuf_free(p);
        tcp_close(tpcb);
        return ERR_MEM;
    }
    hs->sent = 0;

    const SensorReading *last_reading = reading_store_get_last(&sensor_readings);
    
    // Verificação de segurança para dados dos sensores
    if (!last_reading && (strstr(req, "/temperature") || strstr(req, "/humidity") || strstr(req, "/atm_pressure") || strstr(req, "/sensor_status"))) {
        printf("AVISO: Nenhuma leitura de sensor disponível\n");
        char json_payload[] = "{\"error\": \"No sensor data available\"}";
        hs->len = snprintf(hs->response, sizeof(hs->response),
            "HTTP/1.1 503 Service Unavailable\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n\r\n%s",
            (int)strlen(json_payload), json_payload);
    }
    // === ROTAS DOS SENSORES ===
    else if (strstr(req, "GET /temperature")) {
        printf("Servindo rota /temperature: %.2f°C\n", last_reading->temperature);
        char json_payload[256];
        int json_len = snprintf(json_payload, sizeof(json_payload), 
            "{\"value\": %.1f, \"unit\": \"°C\", \"timestamp\": %lu}", 
            last_reading->temperature, 
            (unsigned long)time(NULL));
        
        hs->len = snprintf(hs->response, sizeof(hs->response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n\r\n%s",
            json_len, json_payload);
    }
    else if(strstr(req, "GET /humidity")) {
        printf("Servindo rota /humidity: %.2f%%\n", last_reading->humidity);
        char json_payload[256];
        int json_len = snprintf(json_payload, sizeof(json_payload), 
            "{\"value\": %.1f, \"unit\": \"%%\", \"timestamp\": %lu}", 
            last_reading->humidity,
            (unsigned long)time(NULL));
        
        hs->len = snprintf(hs->response, sizeof(hs->response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n\r\n%s",
            json_len, json_payload);
    }
    else if(strstr(req, "GET /atm_pressure")) {
        printf("Servindo rota /atm_pressure: %.2f hPa\n", last_reading->pressure);
        char json_payload[256];
        int json_len = snprintf(json_payload, sizeof(json_payload), 
            "{\"value\": %.1f, \"unit\": \"hPa\", \"timestamp\": %lu}", 
            last_reading->pressure,
            (unsigned long)time(NULL));
        
        hs->len = snprintf(hs->response, sizeof(hs->response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n\r\n%s",
            json_len, json_payload);
    }
    // === ROTA PARA STATUS GERAL DOS SENSORES ===
    else if(strstr(req, "GET /sensor_status")) {
        printf("Servindo rota /sensor_status\n");
        LimitCheckResult check_result = sensor_limits_check_all(&sensor_limits, 
                                                               last_reading->temperature, 
                                                               last_reading->humidity, 
                                                               last_reading->pressure);
        
        char json_payload[512];
        int json_len = snprintf(json_payload, sizeof(json_payload), 
            "{"
            "\"temperature\": {\"value\": %.1f, \"ok\": %s},"
            "\"humidity\": {\"value\": %.1f, \"ok\": %s},"
            "\"pressure\": {\"value\": %.1f, \"ok\": %s},"
            "\"all_ok\": %s,"
            "\"alert_message\": \"%s\","
            "\"timestamp\": %lu"
            "}", 
            last_reading->temperature, check_result.temperature_ok ? "true" : "false",
            last_reading->humidity, check_result.humidity_ok ? "true" : "false",
            last_reading->pressure, check_result.pressure_ok ? "true" : "false",
            check_result.all_ok ? "true" : "false",
            check_result.alert_message,
            (unsigned long)time(NULL));
        
        hs->len = snprintf(hs->response, sizeof(hs->response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n\r\n%s",
            json_len, json_payload);
    }
    // === ROTA GET PARA OBTER LIMITES ===
    else if(strstr(req, "GET /limits")) {
        printf("Servindo rota /limits\n");
        char json_payload[512];
        int json_len = snprintf(json_payload, sizeof(json_payload),
            "{"
            "\"min_temp\": %.1f,"
            "\"max_temp\": %.1f,"
            "\"min_hum\": %.1f,"
            "\"max_hum\": %.1f,"
            "\"min_press\": %.1f,"
            "\"max_press\": %.1f,"
            "\"alert_enabled\": %s"
            "}",
            sensor_limits.min_temp, sensor_limits.max_temp,
            sensor_limits.min_humidity, sensor_limits.max_humidity,
            sensor_limits.min_pressure, sensor_limits.max_pressure,
            sensor_limits.alert_enabled ? "true" : "false");
        
        hs->len = snprintf(hs->response, sizeof(hs->response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n\r\n%s",
            json_len, json_payload);
    }
    // === ROTA POST PARA DEFINIR LIMITES ===
    else if(strstr(req, "POST /limits")) {
        printf("Processando POST /limits\n");
        // Encontra o início do corpo da requisição (após \r\n\r\n)
        char* body = strstr(req, "\r\n\r\n");
        if (body) {
            body += 4; // Pula os \r\n\r\n
            printf("Body da requisição: %s\n", body);
            
            float min_temp, max_temp, min_hum, max_hum, min_press, max_press;
            bool success = true;
            
            // Extrai os valores do JSON
            success &= extract_json_float(body, "min_temp", &min_temp);
            success &= extract_json_float(body, "max_temp", &max_temp);
            success &= extract_json_float(body, "min_hum", &min_hum);
            success &= extract_json_float(body, "max_hum", &max_hum);
            success &= extract_json_float(body, "min_press", &min_press);
            success &= extract_json_float(body, "max_press", &max_press);
            
            if (success) {
                printf("Atualizando limites: T[%.1f-%.1f], H[%.1f-%.1f], P[%.1f-%.1f]\n",
                       min_temp, max_temp, min_hum, max_hum, min_press, max_press);
                
                // Atualiza os limites
                sensor_limits_set_all(&sensor_limits, 
                                    min_temp, max_temp,
                                    min_hum, max_hum, 
                                    min_press, max_press);
                
                // Salva os limites
                sensor_limits_save(&sensor_limits);
                
                char json_payload[] = "{\"status\": \"success\", \"message\": \"Limites atualizados\"}";
                hs->len = snprintf(hs->response, sizeof(hs->response),
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: application/json\r\n"
                    "Content-Length: %d\r\n"
                    "Connection: close\r\n\r\n%s",
                    (int)strlen(json_payload), json_payload);
            } else {
                printf("ERRO: Falha ao extrair dados JSON\n");
                char json_payload[] = "{\"status\": \"error\", \"message\": \"Dados inválidos\"}";
                hs->len = snprintf(hs->response, sizeof(hs->response),
                    "HTTP/1.1 400 Bad Request\r\n"
                    "Content-Type: application/json\r\n"
                    "Content-Length: %d\r\n"
                    "Connection: close\r\n\r\n%s",
                    (int)strlen(json_payload), json_payload);
            }
        } else {
            printf("ERRO: Corpo da requisição não encontrado\n");
            char json_payload[] = "{\"status\": \"error\", \"message\": \"Corpo da requisição não encontrado\"}";
            hs->len = snprintf(hs->response, sizeof(hs->response),
                "HTTP/1.1 400 Bad Request\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: %d\r\n"
                "Connection: close\r\n\r\n%s",
                (int)strlen(json_payload), json_payload);
        }
    }
    // === ROTA PARA ALTERNAR ALERTAS ===
    else if(strstr(req, "POST /toggle_alerts")) {
        printf("Alternando alertas\n");
        sensor_limits.alert_enabled = !sensor_limits.alert_enabled;
        sensor_limits_save(&sensor_limits);
        
        char json_payload[128];
        int json_len = snprintf(json_payload, sizeof(json_payload),
            "{\"status\": \"success\", \"alert_enabled\": %s}",
            sensor_limits.alert_enabled ? "true" : "false");
        
        hs->len = snprintf(hs->response, sizeof(hs->response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n\r\n%s",
            json_len, json_payload);
    }
    // === ROTA PARA RESETAR LIMITES AOS PADRÕES ===
    else if(strstr(req, "POST /reset_limits")) {
        printf("Resetando limites aos padrões\n");
        sensor_limits_init(&sensor_limits);
        sensor_limits_save(&sensor_limits);
        
        char json_payload[] = "{\"status\": \"success\", \"message\": \"Limites resetados aos padrões\"}";
        hs->len = snprintf(hs->response, sizeof(hs->response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n\r\n%s",
            (int)strlen(json_payload), json_payload);
    }
    else {
        printf("Servindo página principal HTML (%zu bytes)\n", strlen(HTML_BODY));
        hs->len = snprintf(hs->response, sizeof(hs->response),
                           "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/html; charset=UTF-8\r\n"
                           "Content-Length: %d\r\n"
                           "Connection: close\r\n\r\n%s",
                           (int)strlen(HTML_BODY), HTML_BODY);
    }

    pbuf_free(p);
    tcp_arg(tpcb, hs);
    tcp_sent(tpcb, http_sent);
    
    // Enviar a primeira parte da resposta
    size_t chunk_size = hs->len > 1400 ? 1400 : hs->len; // Valor seguro menor que TCP_MSS
    
    err_t write_err = tcp_write(tpcb, hs->response, chunk_size, TCP_WRITE_FLAG_COPY);
    if (write_err != ERR_OK) {
        printf("ERRO: Falha ao escrever resposta TCP: %d\n", write_err);
        free(hs);
        tcp_arg(tpcb, NULL);
        tcp_sent(tpcb, NULL);
        tcp_close(tpcb);
        return write_err;
    }
    
    err_t output_err = tcp_output(tpcb);
    if (output_err != ERR_OK) {
        printf("ERRO: Falha ao enviar resposta TCP: %d\n", output_err);
    }
    
    return ERR_OK;
}

static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    if (err != ERR_OK || newpcb == NULL) {
        printf("ERRO: Falha na conexão TCP: %d\n", err);
        return ERR_VAL;
    }
    
    tcp_recv(newpcb, http_recv);
    tcp_err(newpcb, NULL); // Define callback de erro se necessário
    return ERR_OK;
}

void start_http_server(void) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("ERRO: Falha ao criar PCB TCP\n");
        return;
    }
    
    err_t bind_err = tcp_bind(pcb, IP_ADDR_ANY, 80);
    if (bind_err != ERR_OK) {
        printf("ERRO: Falha ao fazer bind na porta 80: %d\n", bind_err);
        tcp_close(pcb);
        return;
    }
    
    pcb = tcp_listen(pcb);
    if (!pcb) {
        printf("ERRO: Falha ao colocar PCB em modo listen\n");
        return;
    }
    
    tcp_accept(pcb, connection_callback);

    printf("✅ Servidor HTTP rodando na porta 80\n");
    if (netif_default) {
        printf("   Acesse no navegador: http://%s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }
}