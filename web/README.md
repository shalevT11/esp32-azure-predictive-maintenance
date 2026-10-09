# web/ — Next.js dashboard

Live dashboard: real-time charts of telemetry + anomaly highlights, over a WebSocket to the backend.

## Scaffold it (run once, from the repo root)
```bash
npx create-next-app@latest web --ts --app --eslint --no-tailwind --no-src-dir --import-alias "@/*"
cd web && npm install socket.io-client
```
(Left unscaffolded on purpose — `create-next-app` writes ~20 files; generate them locally so the
lockfile matches your Node version.)

## Wire the live feed
```ts
import { io } from 'socket.io-client';
const socket = io('http://localhost:3000');
socket.on('sample',  (s) => { /* append to chart series */ });
socket.on('anomaly', (s) => { /* flag it */ });
```

## Build order
1. One line chart that appends `sample` events live.
2. Anomaly markers from the `anomaly` event.
3. Polish: multiple sensors, time-range, device status.
