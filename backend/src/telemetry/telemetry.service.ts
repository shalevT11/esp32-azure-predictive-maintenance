import { Injectable, Logger } from '@nestjs/common';

export interface Sample {
  seq: number;
  tsMs: number;
  value: number;
}

@Injectable()
export class TelemetryService {
  private readonly log = new Logger(TelemetryService.name);

  /**
   * send every call to the gateway, and he broadcast to the dashboard
   * TODO: connect to the Azure IoT Hub Event Hub-compatible endpoint
   * now we use fake calls
   */
  async start(onSample: (s: Sample) => void): Promise<void> {
    let seq = 0;          //counter for the samples
    setInterval(() => {   //one call every sec
      const value = 1.0 + Math.random() * 0.3; //fake value: base 1.0 + random noise
      const sample: Sample = { seq: seq++, tsMs: Date.now(), value};
      onSample(sample); //the gateway broadcast to the dashboard
    }, 1000);

  }

  /**
   * TODO: persist to TimescaleDB (hypertable) here.
   */
  async persist(s: Sample): Promise<void> {
    // no-op for now
  }

  /**
   * Anomaly detection — the differentiator.
   * TODO: start with a rolling z-score; later swap in Azure AI / Gemini.
   * Returns true if the sample looks anomalous.
   */
  isAnomaly(s: Sample): boolean {
    return false;
  }
}
