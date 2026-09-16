import { Module } from '@nestjs/common';
import { AppController } from './app.controller';
import { AppService } from './app.service';
import { MqttService } from './mqtt/mqtt.service';
import { MeasurementsService } from './measurements/measurements.service';
import { PrismaClient } from '@prisma/client';
import { PrismaService } from './prisma/prisma.service';

@Module({
  imports: [],
  controllers: [AppController],
  providers: [AppService, MqttService, MeasurementsService, PrismaService],
})
export class AppModule {}
