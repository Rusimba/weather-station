import { Test, TestingModule } from '@nestjs/testing';
import { MeasurementsGateway } from './measurements.gateway';

describe('MeasurementsGateway', () => {
  let gateway: MeasurementsGateway;

  beforeEach(async () => {
    const module: TestingModule = await Test.createTestingModule({
      providers: [MeasurementsGateway],
    }).compile();

    gateway = module.get<MeasurementsGateway>(MeasurementsGateway);
  });

  it('should be defined', () => {
    expect(gateway).toBeDefined();
  });
});
