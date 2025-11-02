# mqtt_connect (ESP32-C3)

## Descripción

Proyecto para ESP32-C3 que lee un sensor DHT11, controla un LED y un botón, muestra información en una pantalla OLED y publica/expone el estado a través de un servidor web local. Está pensado como ejemplo funcional para conectar sensores básicos y una UI web ligera.

El código usa el framework ESP-IDF (también empaquetado para PlatformIO en este repo).

## Características principales

- Lectura periódica de temperatura y humedad desde un DHT11.
- Tarea en segundo plano que actualiza las lecturas del DHT11 cada 5 segundos.
- Pantalla OLED I2C para mostrar estado y mensajes (splash, estado WiFi, etc.).
- Botón con debounce y contador de pulsaciones; al presionar el botón se alterna el LED.
- Servidor web integrado con UI para ver estado, datos del sensor y controlar el LED.
- Código organizado en módulos: `hardware`, `wifi_config`, `web_server`, `oled`, `esp32-dht11`.

## Librerías y créditos

- La implementación del DHT11 utilizada en este repositorio proviene de la librería/implementación de: `abdellah2288/esp32-dht11` (autor: abdellah2288). Su código se incluye como `src/esp32-dht11.c` y `include/esp32-dht11.h`.
- Usa ESP-IDF (o PlatformIO con framework espidf) y las librerías estándar del SDK.

## Diagrama de pines (por defecto, ver `include/hardware.h` y `include/oled.h`)

- LED (salida): GPIO 2
- Botón (entrada, pull-up): GPIO 1
- DHT11 (data): GPIO 0
- OLED (I2C): SDA = GPIO 5, SCL = GPIO 6 (dirección 0x3C)

Nota: Ajusta `DHT11_GPIO`, `LED_GPIO`, `BUTTON_GPIO` o los pines I2C en `include/*.h` según tu conexión física.

## Procesos y comportamiento interno

- Inicialización (en `app_main` / `main.c`):
  - Inicializa hardware y periféricos (GPIO, I2C, OLED).
  - Muestra pantalla de bienvenida en OLED.
  - Intenta conectar a WiFi (funciones en `wifi_config.c`).
  - Si WiFi está OK: inicia servidor web (`web_server.c`) y muestra la IP en el OLED.
  - Si falla WiFi: entra en modo local y muestra mensaje en OLED.

- Lectura del DHT11 (en `hardware.c`):
  - Se crea la tarea `dht_task` (Stack 2048 bytes, prioridad 5) que ejecuta `dht11_read()` cada 5 segundos.
  - Tras una lectura exitosa guarda `s_last_temperature` y `s_last_humidity` y marca el sensor como válido; en caso contrario marca como no válido.
  - La implementación del DHT11 maneja el protocolo bit a bit del sensor y verifica checksum.

- Botón y LED (en `hardware.c`):
  - `hardware_update()` hace debounce del botón con un `DEBOUNCE_DELAY` de 50 ms.
  - En flanco de pulsación incrementa el contador `press_count` y alterna el LED.

- Servidor web (en `web_server.c`):
  - Rutas principales:
    - `/` - Página HTML con UI y controles (UTF-8).
    - `/status` - JSON con estado actual: LED, botón, IP, RSSI, temperatura, humedad y si el sensor es válido.
    - `/led` - POST para controlar el LED (acciones: 0=OFF, 1=ON, 2=TOGGLE).
  - La UI realiza peticiones periódicas (cada 3s) para actualizar estado.

## Archivos relevantes

- `src/esp32-dht11.c`, `include/esp32-dht11.h` — implementación DHT11 (fuente: abdellah2288/esp32-dht11).
- `src/hardware.c`, `include/hardware.h` — manejo de GPIO, DHT task, LED y botón.
- `src/oled.c`, `include/oled.h` — drivers y utilidades OLED (I2C).
- `src/wifi_config.c`, `include/wifi_config.h` — conexión WiFi y utilidades.
- `src/web_server.c`, `include/web_server.h` — servidor HTTP y endpoints.

## Cómo compilar y flashear

Este proyecto puede compilarse con PlatformIO (si usas el `platformio.ini` incluído) o con el flujo estándar de ESP-IDF.

Ejemplos (desde la raíz del proyecto):

Con PlatformIO:

```bash
pio run        # compilar
pio run -t upload   # compilar y flashear
pio device monitor  # monitor serie
```

Con ESP-IDF (cmake):

```bash
idf.py build
idf.py -p /dev/ttyUSB0 flash
idf.py -p /dev/ttyUSB0 monitor
```

Asegúrate de seleccionar el puerto serie correcto para tu placa.

## Configuración WiFi y ajustes

- La configuración de red se gestiona en `wifi_config.c` / `include/wifi_config.h`. Modifica SSID/PSK o el método de provisión que uses.

## Buenas prácticas y notas

- Si el DHT11 devuelve lecturas inconsistentes, revisa la conexión (pull-up si aplica) y el pin configurado en `DHT11_GPIO`.
- La tarea de DHT está configurada para 5 segundos entre lecturas; al ser un sensor lento, no es recomendable solicitar lecturas más frecuentes.
- Si añades MQTT u otras integraciones, respeta el uso de tareas y colas para evitar bloquear el loop principal.

## Licencia

Revisa la licencia del código original y la de las dependencias. Este repositorio incluye implementaciones de terceros (como la de `abdellah2288/esp32-dht11`) — respeta sus condiciones.

---

Si quieres que el README incluya una sección de ejemplo de payloads MQTT, diagramas de conexión en ASCII o instrucciones de depuración específicas, dímelo y lo añado.
