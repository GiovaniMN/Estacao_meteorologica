import { useEffect, useState } from "react";
import { WeatherCard } from "./WeatherCard";
import { Thermometer, Droplets, CloudRain, Gauge, Mountain } from "lucide-react";
import { database } from "../firebaseConfig";
import { ref, onValue, off } from "firebase/database";
import WeatherChart from "@/components/Grafico";

interface WeatherData {
  temperatura: number;
  umidade: number;
  chuva_mm: number;
  pressao: number;
  altitude: number;
}

interface ChartData {
  time: string;
  value: number;
}

const chartColors = {
  temperature: "hsl(25, 85%, 55%)",
  humidity: "hsl(200, 85%, 50%)",
  pressure: "hsl(270, 75%, 60%)",
  altitude: "hsl(120, 60%, 45%)",
  precipitation: "hsl(220, 85%, 55%)",
};

const MAX_DATA_POINTS = 30;

export function WeatherDashboard() {
  const [weatherData, setWeatherData] = useState<WeatherData | null>(null);
  const [historicalData, setHistoricalData] = useState<Record<keyof WeatherData, ChartData[]>>({
    temperatura: [],
    umidade: [],
    chuva_mm: [],
    pressao: [],
    altitude: [],
  });
  const [isLoading, setIsLoading] = useState(true);
  const [lastUpdate, setLastUpdate] = useState<Date | null>(null);

  useEffect(() => {
    const weatherRef = ref(database, "sensores/");

    const unsubscribe = onValue(
      weatherRef,
      (snapshot) => {
        const data = snapshot.val();
        if (data) {
          setWeatherData(data);
          setLastUpdate(new Date());

          const now = new Date().toLocaleTimeString();
          setHistoricalData((prev) => {
            const newHistoricalData = { ...prev };
            for (const key in data) {
              if (Object.prototype.hasOwnProperty.call(newHistoricalData, key)) {
                const newPoint = { time: now, value: data[key] };
                const dataArray = [...newHistoricalData[key as keyof WeatherData], newPoint];
                if (dataArray.length > MAX_DATA_POINTS) {
                  dataArray.shift();
                }
                newHistoricalData[key as keyof WeatherData] = dataArray;
              }
            }
            return newHistoricalData;
          });
        }
        setIsLoading(false);
      },
      (error) => {
        console.error("Erro ao buscar dados do Firebase:", error);
        setIsLoading(false);
      }
    );

    return () => off(weatherRef, "value", unsubscribe);
  }, []);

  const formatValue = (value: number | undefined, decimals: number = 1): string => {
    if (value === undefined || value === null) return "--";
    return value.toFixed(decimals);
  };

  return (
    <div className="flex flex-col min-h-screen bg-background">
      {/* Conteúdo principal */}
      <main className="flex-grow">
        <div className="container mx-auto px-4 py-8 max-w-7xl">
          {/* Header */}
          <div className="text-center mb-12">
            <h1 className="text-4xl md:text-5xl font-bold mb-4 text-white">
              Estação Meteorológica
            </h1>
            <p className="text-lg texto mb-2">
              Monitoramento em tempo real das condições atmosféricas
            </p>
            {lastUpdate && (
              <p className="text-sm texto">
                Última atualização: {lastUpdate.toLocaleString("pt-BR")}
              </p>
            )}
          </div>

          {/* Weather Cards Grid */}
          <div className="weather-grid">
            <WeatherCard
              title="Temperatura"
              value={formatValue(weatherData?.temperatura)}
              unit="°C"
              icon={Thermometer}
              color="temperature"
              isLoading={isLoading}
            >
              <WeatherChart
                data={historicalData.temperatura}
                dataKey="value"
                unit=""
                stroke={chartColors.temperature}
                title="Temperatura em Tempo Real (°C)"
              />
            </WeatherCard>

            <WeatherCard
              title="Umidade"
              value={formatValue(weatherData?.umidade, 0)}
              unit="%"
              icon={Droplets}
              color="humidity"
              isLoading={isLoading}
            >
              <WeatherChart
                data={historicalData.umidade}
                dataKey="value"
                unit=""
                stroke={chartColors.humidity}
                title="Umidade em Tempo Real (%)"
              />
            </WeatherCard>

            <WeatherCard
              title="Pressão"
              value={formatValue(weatherData?.pressao, 0)}
              unit="hPa"
              icon={Gauge}
              color="pressure"
              isLoading={isLoading}
            >
              <WeatherChart
                data={historicalData.pressao}
                dataKey="value"
                unit=""
                stroke={chartColors.pressure}
                title="Pressão em Tempo Real (hPa)"
              />
            </WeatherCard>

            <WeatherCard
              title="Altitude"
              value={formatValue(weatherData?.altitude, 0)}
              unit="m"
              icon={Mountain}
              color="altitude"
              isLoading={isLoading}
            >
              <WeatherChart
                data={historicalData.altitude}
                dataKey="value"
                unit=""
                stroke={chartColors.altitude}
                title="Altitude em Tempo Real (m)"
              />
            </WeatherCard>

            <WeatherCard
              title="Precipitação"
              value={formatValue(weatherData?.chuva_mm, 0)}
              unit="mm"
              icon={CloudRain}
              color="precipitation"
              isLoading={isLoading}
            >
              <WeatherChart
                data={historicalData.chuva_mm}
                dataKey="value"
                unit=""
                stroke={chartColors.precipitation}
                title="Precipitação em Tempo Real (mm)"
              />
            </WeatherCard>
          </div>
        </div>
      </main>

      {/* Footer fixado no fim */}
      <footer className="footer text-center pt-8 pb-6 border-t border-white/20 w-full">
        <p className="text-sm text-white">
          &copy; {new Date().getFullYear()} Todos os direitos reservados.
        </p>
        <div className="mt-4 text-sm text-white space-y-1">
          <p>Desenvolvedores:</p>
          <p>Adriano Fernandes Scarabelli</p>
          <p>Giovani Martinho do Nascimento</p>
        </div>
        <p className="text-xs text-white mt-4">
          Projeto desenvolvido para aprovação de horas de estágio prestadas à faculdade
        </p>
      </footer>
    </div>
  );
}
