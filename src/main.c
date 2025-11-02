#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "oled.h"
#include "hardware.h"
#include "wifi_config.h"
#include "web_server.h"
#include "nvs_flash.h"
#include "mqtt_client.h"

static const char *TAG = "MAIN";
static esp_mqtt_client_handle_t mqtt_client = NULL;

// Manejador de eventos MQTT
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Conectado al broker");
            // Suscribirse a tópicos si es necesario
            esp_mqtt_client_subscribe(event->client, "test/server/cmd", 0);
            break;
            
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT Desconectado del broker");
            break;
            
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT Suscrito al tópico, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT Desuscrito del tópico, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT Mensaje publicado, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT Datos recibidos");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
            
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT Error");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGE(TAG, "Error de última conexión al broker = %d", event->error_handle->esp_transport_sock_errno);
                ESP_LOGE(TAG, "Reporte detallado del error = %s", strerror(event->error_handle->esp_transport_sock_errno));
            }
            break;
            
        default:
            ESP_LOGI(TAG, "Otro evento MQTT id:%d", event->event_id);
            break;
    }
}

// Función para inicializar el cliente MQTT
static void mqtt_init(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.transport = MQTT_TRANSPORT_OVER_TCP,
        .broker.address.hostname = "37.27.243.58",
        .broker.address.port = 1883,
        .session.protocol_ver = MQTT_PROTOCOL_V_3_1_1,
        .session.keepalive = 60,
        .credentials.client_id = "ESP32C3_CLIENT"
    };
    
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "Error al crear el cliente MQTT");
        return;
    }
    
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_err_t err = esp_mqtt_client_start(mqtt_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al iniciar el cliente MQTT: %s", esp_err_to_name(err));
    }
}
void app_main(void)
{
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
    
    // 4. Inicializar MQTT después de la conexión WiFi
    if (wifi_ok) {
        ESP_LOGI(TAG, "🔄 Iniciando cliente MQTT...");
        mqtt_init();
    }
    
    // 5. Bucle principal
    ESP_LOGI(TAG, "🔄 Iniciando bucle principal...");
    
    uint32_t last_mqtt_publish = 0;
    char mqtt_data[128];
    
    while(1) {
        hardware_update();
        
        // Mostrar estado actual
        oled_show_button_debug(button_read(), led_get_state());
        
        // Publicar datos cada 5 segundos si MQTT está disponible
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (mqtt_client && wifi_ok && (now - last_mqtt_publish >= 5000)) {
            // Preparar datos en formato JSON
            snprintf(mqtt_data, sizeof(mqtt_data),
                    "{\"led\":%d,\"button\":%d,\"temperature\":%.1f,\"humidity\":%.1f,\"sensor_valid\":%d}",
                    led_get_state(),
                    button_read(),
                    hardware_get_temperature(),
                    hardware_get_humidity(),
                    hardware_sensor_valid());
                    
            // Publicar en el topic
            int msg_id = esp_mqtt_client_publish(mqtt_client, "test/server", mqtt_data, 0, 1, 0);
            if (msg_id != -1) {
                ESP_LOGI(TAG, "Mensaje MQTT enviado: %s", mqtt_data);
            }
            
            last_mqtt_publish = now;
        }
        
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}