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
   * TODO: connect to the Azure IoT Hub Event Hub-compatible endpoint
   * (@azure/event-hubs) and call onSample() for each received message.
   * For Stage 3 you can start by POSTing fake samples to a test endpoint.
   */
  async start(onSample: (s: Sample) => void): Promise<void> {
    this.log.warn('IoT Hub consumer not wired yet — see TODO');
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
