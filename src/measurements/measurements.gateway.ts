import { WebSocketServer, SubscribeMessage, WebSocketGateway, ConnectedSocket, MessageBody } from '@nestjs/websockets';
import { Server, Socket } from 'socket.io';
import { Measurement } from '@prisma/client';

@WebSocketGateway({
  cors: { origin: process.env.FRONTEND_URL ?? 'http://localhost:5173' },
})
export class MeasurementsGateway {
  @WebSocketServer() server: Server;
  @SubscribeMessage('subscribe')
  handleSubscribe(
    @ConnectedSocket() client: Socket,
    @MessageBody() data: { deviceId: string },
  ) {
    client.join(data.deviceId);
  }
  broadcastMeasurement(deviceId: string, measurement: Measurement) {
    this.server.to(deviceId).emit('measurement', measurement);
  }
}
