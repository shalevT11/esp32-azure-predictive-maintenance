import * as dotenv from 'dotenv';
import { join } from 'path';
import { NestFactory } from '@nestjs/core';
import { AppModule } from './app.module';

dotenv.config({ path: join(__dirname, '..', '.env') }); // absolute path → independent of cwd

async function bootstrap() {
  const app = await NestFactory.create(AppModule);
  app.enableCors(); // dashboard runs on a different origin in dev
  await app.listen(process.env.PORT ?? 4321);
}
bootstrap();
