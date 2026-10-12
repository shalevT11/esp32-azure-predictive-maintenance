# ESP32 → Azure IoT · Predictive Maintenance (Cloud-to-Silicon)

An **end-to-end IoT system**: an **ESP32** reads a 9-DOF IMU over **I2C at the register level**,
streams vibration telemetry **securely to Azure IoT Hub over MQTT/TLS**, a **NestJS** backend ingests
it, and a **Next.js** dashboard shows it live while a **rolling z-score** flags anomalies in real time.

Shake the board → the acceleration spikes → the anomaly lights up red on the dashboard, having
travelled device → cloud → backend → browser.

## Architecture

```
ICM-20948 ──I2C──▶ ESP32 (ESP-IDF) ──WiFi/MQTT/TLS──▶ Azure IoT Hub
 (accel/gyro/mag)   SNTP · SAS auth                    │
                                                        ▼
            browser ◀──WebSocket── NestJS (EventHubConsumerClient)
          Next.js + Recharts        rolling z-score anomaly detection
          live chart + red dots
```

## Tech stack

| Layer | What | Stack |
| --- | --- | --- |
| `firmware/` | ESP32 device code | ESP-IDF 5.4, FreeRTOS, I2C, esp-mqtt + TLS, SNTP |
| `backend/`  | Telemetry ingest + anomaly detection | NestJS, `@azure/event-hubs`, socket.io |
| `web/`      | Live dashboard | Next.js (App Router), Recharts |
| `simulator/`| Stand-in device (dev/testing) | Node, `azure-iot-device` (MQTT) |

## How it works

**Device (`firmware/`)**
- Register-level I2C bring-up of the **ICM-20948**: bus scan, `WHO_AM_I` handshake, wake from sleep
  via **read-modify-write** on `PWR_MGMT_1` (bit-mask), and a **burst read** of the accelerometer.
- Each sample's X/Y/Z high/low bytes are combined into signed `int16` (big-endian, 2's complement),
  reduced to an acceleration **magnitude** in g (≈ 1.0 at rest).
- Connects to WiFi, syncs time over **SNTP** (required for TLS cert validation), then connects to
  **Azure IoT Hub over MQTT/TLS** (`esp-mqtt`, built-in CA bundle, **SAS-token** auth) and publishes
  `{seq, value}` to `devices/<id>/messages/events/`.

**Cloud (`backend/` + `web/`)**
- NestJS subscribes to the IoT Hub Event Hub-compatible endpoint (`EventHubConsumerClient`), and
  broadcasts each sample to the browser over a WebSocket.
- A **rolling-window z-score** (`|z| > 3`) flags anomalies; the Next.js dashboard renders a live chart
  and paints flagged points red.

## Running it

Secrets live in `firmware/main/secrets.h` and `backend/.env` (both gitignored — see `.env.example`).

```bash
# backend
cd backend && npm install && npm run start:dev      # :4321

# dashboard
cd web && npm install && npm run dev -- -p 3333      # http://localhost:3333

# firmware: open in VS Code + ESP-IDF, set port, Build/Flash/Monitor
# (or run simulator/send.js as a stand-in device)
```

## Engineering notes (the depth that matters)

The hardware-facing core is written by hand, from the datasheet — not pulled from a library:
- **Register-level I2C** — addressing, write-then-read, burst reads, bit masks (`&= ~(1<<n)`).
- **Byte/endianness handling** — combining high/low bytes into signed 16-bit values.
- **Secure cloud connectivity** — MQTT over TLS, server verification via CA bundle, SAS-token auth,
  and SNTP time sync as its prerequisite.

## License

MIT
