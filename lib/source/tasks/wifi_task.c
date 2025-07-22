#include "wifi.h"
#include "tasks.h"
#include "server.h"
#include "globals.h"

void vTaskWiFi(void *params)
{
    // Inicialização do Wi-Fi
    printf("Inicializando Wi-Fi...\n");
    while(cyw43_arch_init()) {
        printf("ERRO: Falha ao inicializar o Wi-Fi! Tentando novamente em 5s...\n");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }

    cyw43_arch_gpio_put(LED_PIN, 0);    // LED apagado inicialmente
    cyw43_arch_enable_sta_mode();       // Modo Station
    wifi_connected = false;
    printf("Conectando à rede: %s\n", WIFI_SSID);
    while(cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 1000)) {
        printf("ERRO: Falha ao conectar ao Wi-Fi. Tentando novamente...\n");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    wifi_connected = true;
    printf("Conectado com sucesso!\n");
    snprintf(ip_address_str, sizeof(ip_address_str), "%s", ip4addr_ntoa(netif_ip4_addr(netif_default)));
    
    start_http_server();

    while (true)
    {
        if (!cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA)) {
            printf("Desconectado! Tentando reconectar...\n");

            cyw43_wifi_leave(&cyw43_state, CYW43_ITF_STA); // Libera conexão atual
            int ret = cyw43_arch_wifi_connect_blocking(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK);

            if (ret != PICO_OK) {
                printf("ERRO: Reconexão falhou! Tentando novamente em 5s...\n");
                vTaskDelay(pdMS_TO_TICKS(5000));
            } else {
                printf("Reconectado com sucesso!\n");
                vTaskDelay(pdMS_TO_TICKS(500));
            }
        } else {
            cyw43_arch_poll();
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }

    cyw43_arch_deinit();
}
