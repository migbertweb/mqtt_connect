#include "oled.h"
#include "fonts.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "hardware.h"

static const char *TAG = "OLED";

// Comandos SSD1306
#define SSD1306_DISPLAYOFF          0xAE
#define SSD1306_DISPLAYON           0xAF
#define SSD1306_SETDISPLAYCLOCKDIV  0xD5
#define SSD1306_SETMULTIPLEX        0xA8
#define SSD1306_SETDISPLAYOFFSET    0xD3
#define SSD1306_SETSTARTLINE        0x40
#define SSD1306_CHARGEPUMP          0x8D
#define SSD1306_MEMORYMODE          0x20
#define SSD1306_SEGREMAP            0xA0
#define SSD1306_COMSCANDEC          0xC8
#define SSD1306_SETCOMPINS          0xDA
#define SSD1306_SETCONTRAST         0x81
#define SSD1306_SETPRECHARGE        0xD9
#define SSD1306_SETVCOMDETECT       0xDB
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_NORMALDISPLAY       0xA6
#define SSD1306_COLUMNADDR          0x21
#define SSD1306_PAGEADDR            0x22

// Buffer para la pantalla
static uint8_t oled_buffer[SCREEN_WIDTH * (SCREEN_HEIGHT / 8)];

// Función privada para escribir comandos
static void oled_write_cmd(uint8_t cmd) {
    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, (OLED_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd_handle, 0x00, true);
    i2c_master_write_byte(cmd_handle, cmd, true);
    i2c_master_stop(cmd_handle);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd_handle, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd_handle);
}

// Función privada para escribir datos
static void oled_write_data(uint8_t *data, size_t len) {
    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, (OLED_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd_handle, 0x40, true);
    i2c_master_write(cmd_handle, data, len, true);
    i2c_master_stop(cmd_handle);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd_handle, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd_handle);
}

void i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

void oled_init(void) {
    vTaskDelay(100 / portTICK_PERIOD_MS);
    
    // Secuencia de inicialización para SSD1306 72x40
    oled_write_cmd(SSD1306_DISPLAYOFF);
    oled_write_cmd(SSD1306_SETDISPLAYCLOCKDIV);
    oled_write_cmd(0x80);
    oled_write_cmd(SSD1306_SETMULTIPLEX);
    oled_write_cmd(0x27); // 39 = 0x27 (40-1)
    oled_write_cmd(SSD1306_SETDISPLAYOFFSET);
    oled_write_cmd(0x00);
    oled_write_cmd(SSD1306_SETSTARTLINE | 0x00);
    oled_write_cmd(SSD1306_CHARGEPUMP);
    oled_write_cmd(0x14);
    oled_write_cmd(SSD1306_MEMORYMODE);
    oled_write_cmd(0x00);
    oled_write_cmd(SSD1306_SEGREMAP | 0x01);
    oled_write_cmd(SSD1306_COMSCANDEC);
    oled_write_cmd(SSD1306_SETCOMPINS);
    oled_write_cmd(0x12);
    oled_write_cmd(SSD1306_SETCONTRAST);
    oled_write_cmd(0xCF);
    oled_write_cmd(SSD1306_SETPRECHARGE);
    oled_write_cmd(0xF1);
    oled_write_cmd(SSD1306_SETVCOMDETECT);
    oled_write_cmd(0x40);
    oled_write_cmd(SSD1306_DISPLAYALLON_RESUME);
    oled_write_cmd(SSD1306_NORMALDISPLAY);
    oled_write_cmd(SSD1306_DISPLAYON);
    
    ESP_LOGI(TAG, "OLED 72x40 inicializado");
}

void oled_clear(void) {
    memset(oled_buffer, 0, sizeof(oled_buffer));
}

void oled_update(void) {
    oled_write_cmd(SSD1306_COLUMNADDR);
    oled_write_cmd(X_OFFSET);
    oled_write_cmd(X_OFFSET + SCREEN_WIDTH - 1);
    oled_write_cmd(SSD1306_PAGEADDR);
    oled_write_cmd(0);
    oled_write_cmd((SCREEN_HEIGHT / 8) - 1);
    oled_write_data(oled_buffer, sizeof(oled_buffer));
}

void oled_set_power(int on) {
    oled_write_cmd(on ? SSD1306_DISPLAYON : SSD1306_DISPLAYOFF);
}

void oled_draw_pixel(int x, int y) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    
    int page = y / 8;
    int bit = y % 8;
    oled_buffer[x + page * SCREEN_WIDTH] |= (1 << bit);
}

void oled_draw_line(int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    
    while (1) {
        oled_draw_pixel(x0, y0);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void oled_draw_rect(int x, int y, int w, int h) {
    oled_draw_line(x, y, x + w, y);
    oled_draw_line(x + w, y, x + w, y + h);
    oled_draw_line(x + w, y + h, x, y + h);
    oled_draw_line(x, y + h, x, y);
}

void oled_draw_fill_rect(int x, int y, int w, int h) {
    for (int i = x; i < x + w; i++) {
        for (int j = y; j < y + h; j++) {
            oled_draw_pixel(i, j);
        }
    }
}

// FUNCIÓN CORREGIDA - Versión simplificada y funcional
void oled_draw_text(int x, int y, const char *text) {
    // Usar el font directamente desde la variable global
    const uint8_t (*font)[5] = font_5x7;
    
    for (int i = 0; text[i] != '\0'; i++) {
        char c = text[i];
        if (c < 32 || c > 126) continue;
        
        int char_x = x + i * 6; // 5px ancho + 1px espacio
        if (char_x >= SCREEN_WIDTH) break;
        
        // Obtener los datos del carácter
        const uint8_t *char_data = font[c - 32];
        
        // Dibujar cada columna del carácter
        for (int col = 0; col < 5; col++) {
            uint8_t col_data = char_data[col];
            
            // Dibujar cada fila (bit) del carácter
            for (int row = 0; row < 7; row++) {
                if (col_data & (1 << row)) {
                    oled_draw_pixel(char_x + col, y + row);
                }
            }
        }
    }
}

// FUNCIÓN CORREGIDA - Texto centrado
void oled_draw_text_centered(int line, const char *text) {
    int text_width = strlen(text) * 6; // 5px + 1px espacio
    int x = (SCREEN_WIDTH - text_width) / 2;
    int y = line * 10; // 7px alto + 3px espacio
    
    if (x < 0) x = 0;
    oled_draw_text(x, y, text);
}

void oled_show_logo(void) {
    oled_clear();
    oled_draw_text_centered(1, "ESP32-C3");
    oled_draw_text_centered(2, "OLED 0.42");
    oled_update();
}

void oled_show_splash_screen(void) {
    oled_clear();
    oled_draw_text_centered(0, "INICIANDO");
    oled_draw_text_centered(2, "SISTEMA");
    oled_update();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
}

// ==================== NUEVAS FUNCIONES DE INTERFAZ ====================

void oled_show_welcome_screen(void) {
    oled_clear();
    oled_draw_text_centered(0, "SISTEMA");
    oled_draw_text_centered(1, "LED + BOTON");
    oled_draw_text_centered(2, "ESP32-C3");
    oled_draw_text_centered(3, "Listo!");
    oled_update();
}

void oled_show_status_screen(led_state_t led_state, uint32_t press_count) {
    char buffer[32];
    
    oled_clear();
    
    // Título
    oled_draw_text_centered(0, "ESTADO");
    
    // Estado del LED
    oled_draw_text(2, 15, "LED:");
    if (led_state == LED_ON) {
        oled_draw_text(30, 15, "ENCENDIDO");
        // Dibujar un indicador visual del LED
        oled_draw_fill_rect(80, 14, 4, 4);
    } else {
        oled_draw_text(30, 15, "APAGADO");
        oled_draw_rect(80, 14, 4, 4);
    }

    // Contador de clics
    oled_draw_text(2, 25, "Clics:");
    snprintf(buffer, sizeof(buffer), "%lu", press_count);
    oled_draw_text(55, 25, buffer);
    
    oled_update();
}

void oled_show_button_debug(led_state_t led_state, button_state_t button_state) {
    oled_clear();
    // oled_draw_text_centered(0, "DEBUG");
    // Estado del LED
    oled_draw_text(2, 0, "LED:");
    if (led_state == LED_ON) {
        oled_draw_text(26, 0, "Encendido");
    } else {
        oled_draw_text(26, 0, "Apagado");
    }
    
    oled_draw_text(2, 10, "GPIO4:");
    oled_draw_text(39, 10, button_state == BUTTON_PRESSED ? "PRESS" : "LIBRE");
    
    oled_draw_text(2, 20, "GPIO3:");
    oled_draw_text(39, 20, led_get_state() ? "ON" : "OFF");

    // Indicador visual del botón
    oled_draw_text(2, 30, "Estado:");
    if (button_state == BUTTON_PRESSED) {
        oled_draw_fill_rect(45, 29, 20, 6);
    } else {
        oled_draw_rect(45, 29, 20, 6);
    }
    
    oled_update();
}

void oled_show_combined_status(button_state_t button_state, led_state_t led_state, 
                              uint32_t press_count, const char* ip, int rssi) {
    char buffer[32];
    
    oled_clear();
    
    // Header con información WiFi
    oled_draw_text_centered(0, ip);

    // Señal WiFi
    // snprintf(buffer, sizeof(buffer), "%d dBm", rssi);
    // oled_draw_text(60, 0, buffer);
    
    // Estado del LED
    oled_draw_text(0, 10, "LED:");
    oled_draw_text(30, 10, led_state ? "ON " : "OFF");
    if (led_state) {
        oled_draw_fill_rect(55, 9, 4, 4); // Indicador visual
    } else {
        oled_draw_rect(55, 9, 4, 4);
    }
    
    // Estado del botón
    oled_draw_text(0, 20, "BOTON:");
    oled_draw_text(40, 20, button_state == BUTTON_PRESSED ? "PRESS" : "FREE");
    if (button_state == BUTTON_PRESSED) {
        oled_draw_fill_rect(75, 19, 4, 4); // Indicador visual
    } else {
        oled_draw_rect(75, 19, 4, 4);
    }
    
    // Contador de pulsaciones
    oled_draw_text(0, 30, "PULS:");
    snprintf(buffer, sizeof(buffer), "%lu", press_count);
    oled_draw_text(35, 30, buffer);
    
    // Instrucciones
    oled_draw_text_centered(4, "Web: http://");
    oled_draw_text_centered(5, ip);
    
    oled_update();
}