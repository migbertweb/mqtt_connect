#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <stdbool.h>
#include <stdint.h>

// Funciones del servidor web
void web_server_start(void);
void web_server_stop(void);

// Estructura para el estado del sistema
typedef struct {
    bool led_state;
    bool button_state;
    uint32_t press_count;
    char* ip_address;
    float temperature;
    float humidity;
    bool sensor_valid;
} system_status_t;

// Función para obtener el estado del sistema
system_status_t web_get_system_status(void);

#endif // WEB_SERVER_H