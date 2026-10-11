import { NestFactory } from '@nestjs/core';
import { AppModule } from './app.module';

async function bootstrap() {
  const app = await NestFactory.create(AppModule);
  app.enableCors(); // dashboard runs on a different origin in dev
  await app.listen(process.env.PORT ?? 4321);
}
bootstrap();
