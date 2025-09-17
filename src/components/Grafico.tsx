import { LineChart, Line, XAxis, YAxis, Tooltip, CartesianGrid, ResponsiveContainer } from "recharts";

interface WeatherChartProps {
  data: { time: string; value: number }[];
  dataKey: string;
  unit: string;
  stroke: string;
  title: string;
}

export default function WeatherChart({ data, dataKey, unit, stroke, title }: WeatherChartProps) {
  return (
    <div className="glass-card shadow-md">
      <h2 className="text-xl font-bold mb-4 pl-5">{title}</h2>
      <ResponsiveContainer width="100%" height={300}>
        <LineChart data={data}>
          <CartesianGrid strokeDasharray="3 3" />
          <XAxis dataKey="time" />
          <YAxis unit={unit} />
          <Tooltip />
          <Line type="monotone" dataKey={dataKey} stroke={stroke} strokeWidth={2} dot={false} />
        </LineChart>
      </ResponsiveContainer>
    </div>
  );
}