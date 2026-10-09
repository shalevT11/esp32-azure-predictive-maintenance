# backend/ — NestJS

Ingests telemetry from Azure IoT Hub, persists it, runs anomaly detection, and streams it to the
dashboard over WebSockets.

## Run
```bash
npm install
npm run start:dev      # http://localhost:3000, WS on same port
```

## Structure
- `src/telemetry/telemetry.service.ts` — IoT Hub consumer + DB persistence + `isAnomaly()` (the AI hook).
- `src/telemetry/telemetry.gateway.ts` — WebSocket gateway; emits `sample` and `anomaly` events.

## Build order
1. Emit fake `sample` events on a timer → confirm the dashboard draws them live.
2. Wire the real Azure IoT Hub consumer (`@azure/event-hubs`) in `telemetry.service.ts`.
3. Add TimescaleDB persistence.
4. Replace the stub `isAnomaly()` with a rolling z-score, then Azure AI / Gemini.

> This layer is your comfort zone (full-stack) — fine to move fast here. Spend the depth on `firmware/`.
