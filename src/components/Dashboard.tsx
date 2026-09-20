import { useState, useEffect } from 'react';
import { io } from 'socket.io-client';
import { Line, LineChart, XAxis, YAxis, Legend, Tooltip, CartesianGrid } from 'recharts';
export interface Measurement{ 
    id: number;
    deviceId: string;
    temperature: number;
    humidity: number;
    recordedAt: string;
    measuredAt: string | null;
}
const formatXAxis = (tickItem: string) => {
  if (!tickItem) return '';
  
  // Превращаем ISO-строку от @db.Timestamptz в объект Date
  const date = new Date(tickItem); 
  
  return new Intl.DateTimeFormat('ru-RU', {
    hour: '2-digit',
    minute: '2-digit',
  }).format(date);
};
export function Dashboard() {
  const [measurements, setMeasurements] = useState<Measurement[]>([]);
  const deviceId = 'livingroom';
  const limit = 200;
  useEffect(() => {
    const loadMeasurements = async () => {
      try {
        const endpoint = `${import.meta.env.VITE_API_URL}/measurements?deviceId=${deviceId}&limit=${limit}`;
        const response = await fetch(endpoint);
        const data: Measurement[] = await response.json();
        setMeasurements(data);
      } catch (err) {
        console.log('FAILED LOADING', err);
      }
    };

    loadMeasurements();
  }, []);
  useEffect(() => {
    const socket = io(import.meta.env.VITE_API_URL);
    socket.on('connect', () => {
        socket.emit('subscribe', { deviceId });
        console.log('Connected to server');
    });
    socket.on('measurement', (newMeasurement: Measurement) => {
      setMeasurements((prev) => [...prev, newMeasurement].slice(-200));
    });

    return () => {
      socket.disconnect();
    };
  }, []);

  return (
    <LineChart width={1000} height={800} data={measurements}>
    <CartesianGrid strokeDasharray="3 3" />
    <Tooltip />
    <Legend />
  <XAxis dataKey="recordedAt" tickFormatter={formatXAxis} />
  <YAxis />
  <Line type="monotone" dataKey="temperature" stroke="#8884d8" />
  <Line type="monotone" dataKey="humidity" stroke="#82ca9d"  />
</LineChart>
  );
}