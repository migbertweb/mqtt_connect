#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <stdbool.h>

// Configuración WiFi (usando tu código que funciona)
#define WIFI_SSID      "Sukuna-78-2.4g"
#define WIFI_PASSWORD  "gMigbert.78"
#define WIFI_MAX_RETRY 15

// Funciones WiFi
bool wifi_init(void);
bool wifi_is_connected(void);
char* wifi_get_ip(void);
int wifi_get_rssi(void);

#endif // WIFI_CONFIG_H