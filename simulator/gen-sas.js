// Generates an Azure IoT Hub SAS token from the device connection string in .env.
// Run: node gen-sas.js   -> copy the HOST / DEVICE / SAS lines into firmware/main/secrets.h
require('dotenv').config();
const crypto = require('crypto');

const cs = process.env.DEVICE_CONNECTION_STRING;
if (!cs) { console.error('DEVICE_CONNECTION_STRING missing in .env'); process.exit(1); }

// parse "HostName=...;DeviceId=...;SharedAccessKey=..."
const parts = {};
for (const seg of cs.split(';')) {
  const i = seg.indexOf('=');
  parts[seg.slice(0, i)] = seg.slice(i + 1);
}
const host = parts.HostName;
const deviceId = parts.DeviceId;
const key = parts.SharedAccessKey;

// SAS token = SharedAccessSignature sr=<uri>&sig=<HMAC-SHA256>&se=<expiry>
const resourceUri = encodeURIComponent(`${host}/devices/${deviceId}`);
const expiry = Math.floor(Date.now() / 1000) + 60 * 60 * 24 * 30; // 30 days
const toSign = `${resourceUri}\n${expiry}`;
const sig = crypto.createHmac('sha256', Buffer.from(key, 'base64')).update(toSign).digest('base64');
const sas = `SharedAccessSignature sr=${resourceUri}&sig=${encodeURIComponent(sig)}&se=${expiry}`;

console.log('\n--- copy these into firmware/main/secrets.h ---\n');
console.log(`#define AZURE_HOST       "${host}"`);
console.log(`#define AZURE_DEVICE_ID  "${deviceId}"`);
console.log(`#define AZURE_SAS_TOKEN  "${sas}"`);
console.log(`\n(expires: ${new Date(expiry * 1000).toLocaleString()})\n`);
