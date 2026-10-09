# firmware/ — ESP32 (ESP-IDF)

## Prereqs
- ESP-IDF v5.x installed (`idf.py` on PATH). VS Code "Espressif IDF" extension recommended.
- For hardware-free dev: QEMU target (`idf.py --preview set-target esp32 && idf.py qemu`).

## Build / flash / monitor
```bash
idf.py set-target esp32
idf.py build
idf.py -p <PORT> flash monitor     # real board
# or, no board:
idf.py qemu monitor                 # emulator
```

## What's here
- `main/app_main.c` — FreeRTOS skeleton: a sensor-read task + a cloud-tx task joined by a queue.
  **The sensor driver and the Azure MQTT publish are left as TODOs for you to implement** — that's
  the register-level / memory-management code interviews probe. Don't let a generator write it.
- `main/idf_component.yml` — declares the `azure-iot-middleware-freertos` dependency.

## Wiring (solder-free)
ESP32 dev board (pre-soldered headers) on the breadboard; sensor breakout wired with F-F/M-F jumpers.
Default I2C pins: SDA=GPIO21, SCL=GPIO22 (change in `app_main.c`).

## Build order (see root README)
1. `blink` + UART log on real board.
2. Two FreeRTOS tasks + queue (dummy data).
3. Real sensor read over I2C (register level) — verify with the logic analyzer.
4. WiFi + TLS + DPS + publish to IoT Hub (with retries).
