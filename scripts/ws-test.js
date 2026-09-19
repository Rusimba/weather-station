const { io } = require('socket.io-client');

const socket = io('http://localhost:3000');

socket.on('connect', () => {
  console.log('connected', socket.id);
  socket.emit('subscribe', { deviceId: 'livingroom' });
});

socket.on('measurement', (data) => {
  console.log('got measurement:', data);
});