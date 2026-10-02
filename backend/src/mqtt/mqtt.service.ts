import { MqttClient, connect } from 'mqtt';
import {
  Injectable,
  Logger,
  OnModuleInit,
  OnModuleDestroy,
} from '@nestjs/common';
import { MeasurementsService } from '../measurements/measurements.service';

@Injectable()
export class MqttService implements OnModuleInit, OnModuleDestroy {
  private client: MqttClient;
  constructor(private readonly measurements: MeasurementsService) {}
  private readonly topic = 'iot/weather/+/telemetry';
  private readonly logger = new Logger(MqttService.name);

  onModuleInit() {
    this.client = connect('mqtt://192.168.0.8:1883');
    this.client.on('connect', () => {
      this.logger.log('MQTT connected');
      this.client.subscribe(this.topic, { qos: 0 }, (err) => {
        if (err) {
          this.logger.error('MQTT subscription failed', err.message);
        } else this.logger.log(`Subscribed to ${this.topic}`);
      });
    });

    this.client.on('message', async (topic, payload) => {
      try {
        this.logger.log(`${topic} -> ${payload.toString()}`);
        const parse = JSON.parse(payload.toString());

        const parts = topic.split('/');
        await this.measurements.save(
          parts[2],
          parse.temperature,
          parse.humidity,
          parse.pressure,
        );
      } catch (err) {
        this.logger.error(err);
      }
    });
    this.client.on('error', (err) => {
      this.logger.error('MQTT error', err.message);
    });
    this.client.on('reconnect', () => {
      this.logger.error('MQTT reconnecting');
    });
  }

  onModuleDestroy() {
    this.client?.end();
    this.logger.log('MQTT connection closed');
  }
}
