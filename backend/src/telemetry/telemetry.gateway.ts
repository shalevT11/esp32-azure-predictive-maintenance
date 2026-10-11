import { OnModuleInit } from '@nestjs/common';
import {
  WebSocketGateway,
  WebSocketServer,
} from '@nestjs/websockets';
import { Server } from 'socket.io';
import { Sample, TelemetryService } from './telemetry.service';

/**
 * Pushes live telemetry to the Next.js dashboard over WebSockets.
 * Subscribe on the client to the "sample" and "anomaly" events.
 */
@WebSocketGateway({ cors: true })
export class TelemetryGateway implements OnModuleInit {
  @WebSocketServer()
  server!: Server;

  constructor(private readonly telemetry: TelemetryService) {}

  onModuleInit(): void {
    // Wire the IoT Hub consumer -> persist -> broadcast pipeline.
    void this.telemetry.start((s: Sample) => this.handleSample(s));
  }

  private async handleSample(s: Sample): Promise<void> {
    await this.telemetry.persist(s);
    this.server.emit('sample', s);
    if (this.telemetry.isAnomaly(s)) {
      this.server.emit('anomaly', s);
    }
  }
}
