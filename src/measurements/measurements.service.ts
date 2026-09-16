import { Injectable, Logger } from '@nestjs/common';
import { PrismaService } from 'src/prisma/prisma.service';

@Injectable()
export class MeasurementsService {
  private readonly logger = new Logger(MeasurementsService.name);
  constructor(private readonly prisma: PrismaService) {}
  async save(deviceId: string, temperature: number, humidity: number) {
    if (deviceId == null || deviceId.trim() === '') {
      this.logger.warn('deviceId TROUBLE');
      return false;
    }
    if (
      Number.isFinite(temperature) == false ||
      temperature < -80 ||
      temperature > 80
    ) {
      this.logger.warn(
        `Invalid temperature ${temperature} for device=${deviceId}`,
      );
      return false;
    }
    if (Number.isFinite(humidity) == false || humidity < 0 || humidity > 101) {
      this.logger.warn(`Invalid humidity ${humidity} for device=${deviceId}`);
      return false;
    }
    try {
      await this.prisma.measurement.create({
        data: {
          recordedAt: new Date(),
          deviceId: deviceId,
          temperature: temperature,
          humidity: humidity,
        },
      });
    } catch (err) {
      this.logger.error(
        `DB write failed device=${deviceId}: ${(err as Error).message}`,
        (err as Error).stack,
      );
      return false;
    }
    this.logger.log(
      `Saved measurement device=${deviceId} t=${temperature} h=${humidity}`,
    );
    return true;
  }
  async history(deviceId: string, limit: number = 200) {
    const rows = await this.prisma.measurement.findMany({
      where: { deviceId },
      orderBy: { recordedAt: 'desc' },
      take: limit,
    });
    return rows.reverse();
  }
}
