#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "oled.h"
#include "hardware.h"
#include "wifi_config.h"
#include "web_server.h"

static const char *TAG = "MAIN";

void app_main(void) {
    ESP_LOGI(TAG, "📡 Iniciando Sistema ESP32-C3");
    
    // 1. Inicializar hardware
    hardware_init();
    i2c_master_init();
    oled_init();
    oled_show_welcome_screen();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    // 2. Conectar WiFi (SOLO UNA VEZ)
    ESP_LOGI(TAG, "📡 Conectando a WiFi...");
    oled_clear();
    oled_draw_text_centered(1, "Conectando WiFi...");
    oled_update();
    
    bool wifi_ok = wifi_init();
    
    if (wifi_ok) {
        char* ip = wifi_get_ip();
        ESP_LOGI(TAG, "✅ WiFi conectado - IP: %s", ip);
        
        // 3. Iniciar servidor web
        ESP_LOGI(TAG, "🌐 Iniciando servidor web...");
        web_server_start();
        
        oled_clear();
        oled_draw_text_centered(0, "WEB SERVER ACTIVE");
        oled_draw_text_centered(1, ip);
        oled_draw_text_centered(2, "Listo!");
        oled_update();
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        
        ESP_LOGI(TAG, "✅ Sistema listo: http://%s", ip);
    } else {
        ESP_LOGE(TAG, "❌ WiFi falló - Modo local");
        oled_clear();
        oled_draw_text_centered(1, "MODO LOCAL");
        oled_draw_text_centered(2, "Sin WiFi");
        oled_update();
        vTaskDelay(8000 / portTICK_PERIOD_MS);
    }
    
    // 4. Bucle principal (sin reconexión automática)
    ESP_LOGI(TAG, "🔄 Iniciando bucle principal...");
    
    while(1) {
        hardware_update();
        
        // Mostrar estado actual (sin intentar reconectar)
        oled_show_button_debug(button_read(), led_get_state());
        
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}