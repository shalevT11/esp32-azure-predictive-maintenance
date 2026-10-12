import { EventHubConsumerClient } from '@azure/event-hubs';
import { Injectable, Logger } from '@nestjs/common';

export interface Sample {
  seq: number;
  tsMs: number;
  value: number;
}

@Injectable()
export class TelemetryService {
  private readonly log = new Logger(TelemetryService.name);
  private readonly window: number[] = []; //the window hold the last 30 sample for cpmparing
  private readonly WINDOW_SIZE = 30;

  /**
   * SUB to the IoT HUB
   * each msg -> Sample -> onSample
   */
  async start(onSample: (s: Sample) => void): Promise<void> {
      const connectionString = process.env.EVENTHUB_CONNECTION_STRING!; //the secret from .env
      const consumerGroup = '$Default';        //consumer group = read cursor; default

      const client = new EventHubConsumerClient(consumerGroup, connectionString);

      client.subscribe({
          processEvents: async (events) => { //events come from cloud in baches
              for(const event of events) {
                const data = event.body; //SDK auto-parges body when contentType is application/json
                const sample: Sample = { seq: data.seq, tsMs: Date.now(), value: data.value };
                onSample(sample); //same contract as the fake source - flow to broadcast
              }
          },
          processError: async (err) => { //if somethig wrong (like exception)
            this.log.error(err);
          },
      });

      this.log.log('Subscribed to IoT, waiting for telemetry...');
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
    if (this.window.length < 15) { //not enough to jugde on problem
      this.window.push(s.value);
      return false;
    }

    //z score cacl how much the last sample is far from the avarage
    let mean = this.window.reduce((acc, x) => acc + x, 0) /this.window.length;
    let variance = this.window.reduce((acc, x) => acc + (x - mean)**2, 0) / this.window.length;
    let std = Math.sqrt(variance);

    if (std===0) { //cant div by 0 (if all values are equals)
      return false;
    }

    const z = (s.value - mean) / std;
    
    this.window.push(s.value); //add after the judge, that the sample wont fall into statistics

    if (this.window.length > this.WINDOW_SIZE) {
      this.window.shift();
    }

    return Math.abs(z) > 3; //3 far from normal, 99.7% is normal, beyond that is NOT OK
  }
}
