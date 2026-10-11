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
   * send every call to the gateway, and he broadcast to the dashboard
   * TODO: connect to the Azure IoT Hub Event Hub-compatible endpoint
   * now we use fake calls
   */
  async start(onSample: (s: Sample) => void): Promise<void> {
    let seq = 0;          //counter for the samples
    setInterval(() => {   //one call every sec
      const spike = Math.random() < 0.05; //jump 5% from the time
      const value = spike ? 1.5 : 1.0 + Math.random() * 0.3; //fake value: base 1.0 + random noise
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
