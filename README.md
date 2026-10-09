# ESP32 → Azure IoT · Predictive Maintenance (Cloud-to-Silicon)

End-to-end IoT system: an **ESP32 (FreeRTOS)** samples a sensor, streams telemetry **securely to
Azure IoT Hub** over MQTT/TLS, a **NestJS** backend ingests and persists it, a **Next.js** dashboard
shows it live, and an **AI layer** flags anomalies to predict hardware failures *before* they happen.

> Portfolio project targeting Microsoft Israel (Azure IoT / Defender for IoT / Cloud-to-Silicon roles).
> Spec & rationale: `../embedded-roadmap/project-azure-iot-flagship.md`.

## Architecture

```
Sensor ──I2C/SPI──▶ ESP32 + FreeRTOS ──MQTT/TLS──▶ Azure IoT Hub ──▶ NestJS ──WS──▶ Next.js
(vibration/temp)    (azure-iot-                      + DPS            + DB         dashboard
                     middleware-freertos)                             + AI anomaly detection
```

## Repo layout

| Dir | What | Stack |
| --- | --- | --- |
| `firmware/` | ESP32 device code | ESP-IDF + FreeRTOS + azure-iot-middleware-freertos |
| `backend/`  | Telemetry ingest + API + AI | NestJS + WebSockets + Postgres/TimescaleDB |
| `web/`      | Live dashboard | Next.js |
| `docs/`     | Architecture & decisions | Markdown |

## Status

🚧 **Stage 0 — scaffolding.** See `docs/architecture.md` and each dir's README for the build plan.

## Engineering notes (the depth that matters)

The hardware-facing core is written by hand, not generated:
- **Sensor driver** — register-level I2C/SPI reads from the datasheet (bit masks, not a library).
- **`azure-iot-middleware-freertos`** — it does **no dynamic allocation**; buffers, TLS setup, and
  **retry/reconnect** logic are managed explicitly. This is the interview-probed part.

See the `CHALLENGES.md` (to be written) for the debug log — the most valuable part of the README.

## License

MIT — see `LICENSE`.
