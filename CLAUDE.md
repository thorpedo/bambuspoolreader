# bambuspoolreader

el proposito del proyecto es utilizar un esp32 ( liligo t-display ) y un lector de nfc para leer los tag de las bobinas de bambu ( color , material , nombre comercial ) . En el esp32 cuando lee la etiqueta debe aparecer en la pantalla esos datos y tambien un indicador de bateria. Instala todas las herramientas que necesites para conectar y programar dicha placa

## Comandos

Firmware en C con ESP-IDF v5.5.6, instalado en `~/esp/esp-idf`. PlatformIO no se usa.

```bash
. ~/esp/esp-idf/export.sh                  # activar ESP-IDF en la shell
idf.py build                               # compilar
idf.py -p /dev/ttyACM0 flash monitor       # grabar y ver el log (salir: Ctrl+])
idf.py -DDEMO_TAGS=1 -p /dev/ttyACM0 flash # demo con bobinas de ejemplo, sin lector
idf.py -DDEMO_TAGS=0 -p /dev/ttyACM0 flash # volver al firmware normal (la opción se recuerda)
gcc -Wall -Wextra -Werror -Imain test/test_host.c main/bambu_tag.c -o /tmp/test_host && /tmp/test_host   # tests en el PC
```

En WSL la placa se conecta desde PowerShell con `usbipd attach --wsl --busid <id>`.

## Estructura

- `main/board.h`: pines de la placa y del lector.
- `main/app_main.c`: bucle principal.
- `main/pn532.c`: lector NFC PN532 por I2C.
- `main/bambu_tag.c`: interpretación de los bloques de la etiqueta; sin dependencias de ESP-IDF.
- `main/bambu_keys.c`: derivación de claves a partir del UID.
- `main/display.c`: pantalla ST7789 con LVGL.
- `main/battery.c`, `main/battery_level.h`: lectura de batería y cálculo del porcentaje.
- `test/test_host.c`: tests de la lógica pura, sin hardware.

## Convenciones

- Código e identificadores en inglés; textos de pantalla y documentación en español.
- La lógica sin hardware va separada de los drivers y lleva test en `test/test_host.c`.

