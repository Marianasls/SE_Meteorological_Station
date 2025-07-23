#include <stdio.h>
#include <math.h>
#include <string.h>

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

// declaração de funções
void gpio_init_all();
void update_display();
void configureWiFi();
double calculate_altitude(double pressure); 


int main() {
    stdio_init_all();
    gpio_init_all();

    // Inicializa o wifi
    configureWiFi();

    // Inicializa o servidor HTTP
    start_http_server();


    // Estrutura para armazenar os dados do sensor
    AHT20_Data data;
    int32_t raw_temp_bmp;
    int32_t raw_pressure;

    char str_tmp1[5];  // Buffer para armazenar a string
    char str_alt[5];  // Buffer para armazenar a string  
    char str_tmp2[5];  // Buffer para armazenar a string
    char str_umi[5];  // Buffer para armazenar a string 

    bool cor = true;

    // Inicialize o armazenamento de leituras
    reading_store_init(&sensor_readings);
    
    while (1) {
        // Leitura do BMP280
        bmp280_read_raw(I2C_PORT, &raw_temp_bmp, &raw_pressure);
        int32_t temperature = bmp280_convert_temp(raw_temp_bmp, &params);
        int32_t pressure = bmp280_convert_pressure(raw_pressure, raw_temp_bmp, &params);

        // Cálculo da altitude
        double altitude = calculate_altitude(pressure);

        printf("Pressao = %.3f kPa\n", pressure / 1000.0);
        printf("Temperatura BMP: = %.2f C\n", temperature / 100.0);
        printf("Altitude estimada: %.2f m\n", altitude);

        // Leitura do AHT20
        if (aht20_read(I2C_PORT, &data)){
            printf("Temperatura AHT: %.2f C\n", data.temperature);
            printf("Umidade: %.2f %%\n\n\n", data.humidity);
        } else {
            printf("Erro na leitura do AHT10!\n\n\n");
        }

        sprintf(str_tmp1, "%.1fC", temperature / 100.0);  // Converte o inteiro em string
        sprintf(str_alt, "%.0fm", altitude);  // Converte o inteiro em string
        sprintf(str_tmp2, "%.1fC", data.temperature);  // Converte o inteiro em string
        sprintf(str_umi, "%.1f%%", data.humidity);  // Converte o inteiro em string  

        // Atualiza o conteúdo do display com animações
        //update_display();
        //  Atualiza o conteúdo do display com animações
        ssd1306_fill(&ssd, !cor);                           // Limpa o display
        ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);       // Desenha um retângulo
        ssd1306_line(&ssd, 3, 25, 123, 25, cor);            // Desenha uma linha
        ssd1306_line(&ssd, 3, 37, 123, 37, cor);            // Desenha uma linha
        ssd1306_draw_string(&ssd, "Estacao", 8, 8); // Desenha uma string
        ssd1306_draw_string(&ssd, "Meteorologica", 8, 16); // Desenha uma string
        ssd1306_draw_string(&ssd, "BMP280  AHT10", 10, 28); // Desenha uma string
        ssd1306_line(&ssd, 63, 25, 63, 60, cor);            // Desenha uma linha vertical
        ssd1306_draw_string(&ssd, str_tmp1, 14, 41);             // Desenha uma string
        ssd1306_draw_string(&ssd, str_alt, 14, 52);             // Desenha uma string
        ssd1306_draw_string(&ssd, str_tmp2, 73, 41);             // Desenha uma string
        ssd1306_draw_string(&ssd, str_umi, 73, 52);            // Desenha uma string
        ssd1306_send_data(&ssd);                            // Atualiza o display
        sleep_ms(500);

        // Depois de ler os sensores e antes de atualizar o display, armazene a leitura
        // Calcule a média da temperatura entre os dois sensores
        float avg_temp = (temperature / 100.0f + data.temperature) / 2.0f;

        // Armazene a leitura na pilha
        reading_store_add(&sensor_readings, avg_temp, data.humidity, pressure / 100.0f);

        cyw43_arch_poll(); // Necessário para manter o Wi-Fi ativo
        sleep_ms(1000);
    }
    
    cyw43_arch_deinit(); // Desliga o Wi-Fi antes de sair
    return 0;
}

void gpio_init_all() {
    // Para ser utilizado o modo BOOTSEL com botão B
    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // I2C do Display funcionando em 400Khz.
    i2c_init(I2C_PORT_DISP, 400 * 1000);
    gpio_set_function(I2C_SDA_DISP, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL_DISP, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA_DISP);                                        // Pull up the data line
    gpio_pull_up(I2C_SCL_DISP);                                        // Pull up the clock line
    
    ssd1306_init(&ssd, DISPLAY_WIDTH, DISPLAY_HEIGHT, false, DISPLAY_ADDRESS, I2C_PORT_DISP); // Inicializa o display
    ssd1306_config(&ssd);                                              // Configura o display
    ssd1306_send_data(&ssd);                                           // Envia os dados para o display

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // Inicializa o I2C dos sensores BMP280 e AHT20
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
	PIO pio = pio0;
	int sm = 0;
	uint offset = pio_add_program(pio, &ws2812_program);
	ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, false);

    clear_buffer();
    set_leds(0, 0, 0); // Limpa a matriz de LEDs
}

// Função para calcular a altitude a partir da pressão atmosférica
double calculate_altitude(double pressure) {
    return 44330.0 * (1.0 - pow(pressure / SEA_LEVEL_PRESSURE, 0.1903));
}

// FUNÇÕES PARA SERVER HTML
const char HTML_BODY[] =
"<!DOCTYPE html><html lang=\"pt-BR\"><head><meta charset=\"UTF-8\" />"
"  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"/><title>Estação Meteorológica</title>"
"  <style>"
"    body { font-family: Arial, sans-serif; margin: 0; padding: 1rem; background: #f0f4f8; }"
"    h1 { text-align: center; }"
"    .card { background: white; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); padding: 1rem; margin: 1rem auto; max-width: 600px; }"
"    .sensor-values { display: flex; justify-content: space-around; font-size: 1.5rem; }"
"    .graph { width: 100%; height: 200px; background: #ddd; margin: 1rem 0; }"
"    form { display: flex; flex-direction: column; gap: 0.5rem; }"
"    label { font-weight: bold; }"
"    input { padding: 0.5rem; font-size: 1rem; }"
"    button { padding: 0.5rem; font-size: 1rem; background: #007bff; color: white; border: none; border-radius: 5px; }"
"    @media (max-width: 500px) { .sensor-values { flex-direction: column; align-items: center; gap: 1rem; } }</style></head>"
"<body>"
"  <h1>Estação Meteorológica</h1>"
"  <div class=\"card\">"
"    <div class=\"sensor-values\">"
"      <div>🌡️ Temp: <span id=\"temp\">--</span>°C</div>"
"      <div>💧 Umid: <span id=\"humidity\">--</span>%</div>"
"      <div>📈 Press: <span id=\"pressure\">--</span> hPa</div>"
"    </div>"
"    <div class=\"graph\" id=\"graph\">[Gráfico placeholder]</div></div>"
"  <div class=\"card\">"
"    <h2>Configurações</h2>"
"    <form id=\"config-form\">"
"      <label for=\"min_temp\">Temp. Mínima:</label>"
"      <input type=\"number\" id=\"min_temp\" name=\"min_temp\" step=\"0.1\" />"
"      <label for=\"max_temp\">Temp. Máxima:</label>"
"      <input type=\"number\" id=\"max_temp\" name=\"max_temp\" step=\"0.1\" />"
"      <label for=\"min_hum\">Umidade Mínima:</label>"
"      <input type=\"number\" id=\"min_hum\" name=\"min_hum\" step=\"0.1\" />"
"      <label for=\"max_hum\">Umidade Máxima:</label>"
"      <input type=\"number\" id=\"max_hum\" name=\"max_hum\" step=\"0.1\" />"
"      <label for=\"min_press\">Pressão Mínima:</label>"
"      <input type=\"number\" id=\"min_press\" name=\"min_press\" step=\"0.1\" />"
"      <label for=\"max_press\">Pressão Máxima:</label>"
"      <input type=\"number\" id=\"max_press\" name=\"max_press\" step=\"0.1\" />"
"      <button type=\"submit\">Salvar</button></form></div>"
"  <script>"
"    async function fetchData() {\n"
"      try {\n"
"        const [tempRes, humRes, presRes, limRes] = await Promise.all([\n"
"          fetch(\"/temperature\"),\n"
"          fetch(\"/humidity\"),\n"
"          fetch(\"/atm_pressure\"),\n"
"          fetch(\"/limits\") ]);\n"
"        const temp = await tempRes.json();\n"
"        const hum = await humRes.json();\n"
"        const pres = await presRes.json();\n"
"        const limits = await limRes.json();\n"
"        document.getElementById(\"temp\").textContent = temp.value ?? \"--\";\n"
"        document.getElementById(\"humidity\").textContent = hum.value ?? \"--\";\n"
"        document.getElementById(\"pressure\").textContent = pres.value ?? \"--\";\n"
"        document.getElementById(\"min_temp\").value = limits.min_temp ?? \"\";\n"
"        document.getElementById(\"max_temp\").value = limits.max_temp ?? \"\";\n"
"        document.getElementById(\"min_hum\").value = limits.min_hum ?? \"\";\n"
"        document.getElementById(\"max_hum\").value = limits.max_hum ?? \"\";\n"
"        document.getElementById(\"min_press\").value = limits.min_press ?? \"\";\n"
"        document.getElementById(\"max_press\").value = limits.max_press ?? \"\";\n"
"      } catch (err) {\n console.error(\"Erro ao buscar dados:\", err);\n}\n}\n"
"    document.getElementById(\"config-form\").addEventListener(\"submit\", async (e) => {\n"
"      e.preventDefault();\n"
"      const min_temp = parseFloat(document.getElementById(\"min_temp\").value);\n"
"      const max_temp = parseFloat(document.getElementById(\"max_temp\").value);\n"
"      const min_hum = parseFloat(document.getElementById(\"min_hum\").value);\n"
"      const max_hum = parseFloat(document.getElementById(\"max_hum\").value);\n"
"      const min_press = parseFloat(document.getElementById(\"min_press\").value);\n"
"      const max_press = parseFloat(document.getElementById(\"max_press\").value);\n"
"      await fetch(\"/limits\", {\n"
"        method: \"POST\",\n"
"        headers: { \"Content-Type\": \"application/json\" },\n"
"        body: JSON.stringify({\n"
"          min_temp, max_temp,\n"
"          min_hum, max_hum,\n"
"          min_press, max_press\n"
"        })\n});\n});"
"    setInterval(fetchData, 3000); fetchData();\n"
"  </script></body></html>\n";

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
    while(cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 1000)) {
        printf("ERRO: Falha ao conectar ao Wi-Fi. Tentando novamente...\n");
        sleep_ms(5000);
    }
    wifi_connected = true;
    printf("Conectado com sucesso!\n");
    snprintf(ip_address_str, sizeof(ip_address_str), "%s", ip4addr_ntoa(netif_ip4_addr(netif_default)));

    printf("Tamanho do HTML_BODY: %zu\n", strlen(HTML_BODY));
    
}
struct http_state
{
    char response[8192];
    size_t len;
    size_t sent;
};

static err_t http_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
    struct http_state *hs = (struct http_state *)arg;
    hs->sent += len;
    if (hs->sent >= hs->len)
    {
        tcp_close(tpcb);
        free(hs);
    }
    return ERR_OK;
}

static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (!p) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    char *req = (char *)p->payload;
    struct http_state *hs = malloc(sizeof(struct http_state));
    if (!hs) {
        pbuf_free(p);
        tcp_close(tpcb);
        return ERR_MEM;
    }
    hs->sent = 0;

    const SensorReading *last_reading = reading_store_get_last(&sensor_readings);

    if (strstr(req, "GET /temperature")) {
        float temperature = last_reading ? last_reading->temperature : 0.0f;
        char json_payload[256];
        int json_len = snprintf(json_payload, sizeof(json_payload), "Valor: %.1f", temperature);
        
        hs->len = snprintf(hs->response, sizeof(hs->response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            json_len, json_payload);
    }
    else if(strstr(req, "GET /atm_pressure")) {
        float pressure = last_reading ? last_reading->pressure : 0.0f;
        char json_payload[256];
        int json_len = snprintf(json_payload, sizeof(json_payload), "Valor: %.1f", pressure);
        
        hs->len = snprintf(hs->response, sizeof(hs->response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            json_len, json_payload);
    }
    else if(strstr(req, "GET /humidity")) {
        float humidity = last_reading ? last_reading->humidity : 0.0f;

        char json_payload[256];
        int json_len = snprintf(json_payload, sizeof(json_payload), "Valor: %.1f", humidity);
        
        hs->len = snprintf(hs->response, sizeof(hs->response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            json_len, json_payload);
    }
    else if(strstr(req, "GET /limits")) {
        char json_payload[256];
        int json_len = snprintf(json_payload, sizeof(json_payload),
            "{\"min\":%.1f,\"max\":%.1f}\r\n",
            12, 12);
        
            hs->len = snprintf(hs->response, sizeof(hs->response),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: %d\r\n"
                "Connection: close\r\n"
                "\r\n"
                "%s",
                json_len, json_payload);
    }
    else {
        hs->len = snprintf(hs->response, sizeof(hs->response),
                           "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/html\r\n"
                           "Content-Length: %d\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "%s",
                           (int)strlen(HTML_BODY), HTML_BODY);
    }

    tcp_arg(tpcb, hs);
    tcp_sent(tpcb, http_sent);
    tcp_write(tpcb, hs->response, hs->len, TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    pbuf_free(p);
    return ERR_OK;
}

static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, http_recv);
    return ERR_OK;
}

void start_http_server(void)
{
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("Erro ao criar PCB TCP\n");
        return;
    }
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Erro ao ligar o servidor na porta 80\n");
        return;
    }
    pcb = tcp_listen(pcb);
    tcp_accept(pcb, connection_callback);
    if (netif_default) {
        printf("Servidor HTTP rodando no endereço %s:80 ...\n", ipaddr_ntoa(&netif_default->ip_addr));

    } else {
        printf("Servidor HTTP rodando na porta 80...\n");

    }
}