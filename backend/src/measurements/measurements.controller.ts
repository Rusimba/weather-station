import {
  Controller,
  DefaultValuePipe,
  Get,
  ParseIntPipe,
  Query,
} from '@nestjs/common';
import { MeasurementsService } from './measurements.service';
@Controller('measurements')
export class MeasurementsController {
  constructor(private readonly measurements: MeasurementsService) {}
  @Get()
  history(
    @Query('deviceId') deviceId: string,
    @Query('limit', new DefaultValuePipe(200), ParseIntPipe) limit: number,
  ) {
    return this.measurements.history(deviceId, limit);
  }
}
