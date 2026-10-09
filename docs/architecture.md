# Architecture & Decisions

## Data flow

```
┌──────────────┐  I2C/SPI   ┌─────────────────────────────┐
│ Sensor       │───────────▶│ ESP32 (FreeRTOS)            │
│ MPU6050 /    │            │  Task: sensor_read (periodic)│
│ BME280       │            │   └─ writes to xQueue        │
└──────────────┘            │  Task: cloud_tx              │
                            │   └─ reads queue, publishes  │
                            │      via azure-iot-middleware│
                            └──────────────┬──────────────┘
                                           │ MQTT 3.1.1 over TLS 1.2
                                           ▼
                            ┌─────────────────────────────┐
                            │ Azure IoT Hub  (+ DPS)       │
                            └──────────────┬──────────────┘
                                           │ Event Hub-compatible endpoint
                                           ▼
┌──────────────┐  WebSocket ┌─────────────────────────────┐
│ Next.js      │◀───────────│ NestJS                      │
│ dashboard    │            │  • IoT Hub consumer          │
│ (live charts)│            │  • TimescaleDB write         │
└──────────────┘            │  • AnomalyService (AI)       │
                            └─────────────────────────────┘
```

## Key decisions (fill in as you build — this log is interview gold)

| Decision | Choice | Why |
| --- | --- | --- |
| MCU | ESP32 (WROOM) | WiFi + FreeRTOS + TLS 1.2 in HW; STM32F407 has no networking |
| RTOS | FreeRTOS (built into ESP-IDF) | Covers the "RTOS layer" of the holy trinity |
| Cloud SDK | `azure-iot-middleware-freertos` | Official MS lib; no malloc → forces real memory discipline |
| Transport | MQTT over TLS | Azure IoT Hub native; learn the handshake, not just "it works" |
| Provisioning | DPS | Secure device identity (Zero Trust keyword for MS) |
| Backend | NestJS | Already in comfort zone; clean DI for the AI service |
| DB | TimescaleDB (Postgres) | Time-series telemetry; hypertables |
| AI | Azure AI / Gemini / local stat model | Anomaly detection = the differentiator |

## Threat model / security (to expand)

- Device identity via DPS (X.509 or SAS).
- TLS 1.2 mutual-ish auth; server cert validation on-device.
- No secrets in firmware source — provisioned at flash time / via DPS.

## Open questions

- [ ] Sensor: vibration (MPU6050) vs environmental (BME280)? Vibration = more realistic predictive-maintenance signal.
- [ ] Anomaly model: statistical (z-score / rolling window) first, AI/LLM second?
- [ ] FOTA story (stretch goal — great System Design interview material).
