import { useEffect, useState } from "react";
import { WeatherCard } from "./WeatherCard";
import {
  Thermometer,
  ThermometerSun,
  ThermometerSnowflake,
  Droplets,
  Droplet,
  Waves,
  Gauge,
  Activity,
  CloudRain,
  CloudDrizzle,
  CloudLightning,
  Sun
} from "lucide-react";
import { database } from "../firebaseConfig";
import { ref, onValue, off } from "firebase/database";
import WeatherChart from "@/components/Grafico";

interface WeatherData {
  temperatura: number;
  umidade: number;
  chuva_nivel: number;
  chuva_intensidade: string;
  chuva_descricao: string;
  pressao: number;
}

interface ChartData {
  time: string;
  value: number;
}

const chartColors = {
  temperature: "red",
  humidity: "blue",
  pressure: "green",
  precipitation: "darkblue",
};

const getTempConfig = (value: number) => {
  if (value <= 20) {
    return { icon: ThermometerSnowflake, color: "red", label: "Frio" };
  }
  if (value >= 30) {
    return { icon: ThermometerSun, color: "red", label: "Quente" };
  }
  return { icon: Thermometer, color: "red", label: "Normal" };
};

const getHumidityConfig = (value: number) => {
  if (value < 40) {
    return { icon: Droplet, color: "blue", label: "Baixa" };
  }
  if (value > 70) {
    return { icon: Waves, color: "blue", label: "Alta" };
  }
  return { icon: Droplets, color: "blue", label: "Normal" };
};

const getPressureConfig = (value: number) => {
  if (value < 1000) {
    return { icon: Activity, color: "green", label: "Baixa" };
  }
  if (value > 1020) {
    return { icon: Activity, color: "green", label: "Alta" };
  }
  return { icon: Gauge, color: "green", label: "Normal" };
};

const getRainConfig = (level: number) => {
  switch (level) {
    case 0:
      return { icon: Sun, color: "darkblue", label: "Sem Chuva" };
    case 1:
      return { icon: CloudDrizzle, color: "darkblue", label: "Garoa" };
    case 2:
      return { icon: CloudRain, color: "darkblue", label: "Fraca" };
    case 3:
      return { icon: CloudRain, color: "darkblue", label: "Moderada" };
    case 4:
      return { icon: CloudLightning, color: "darkblue", label: "Forte" };
    case 5:
      return { icon: CloudLightning, color: "darkblue", label: "Intensa" };
    case 6:
      return { icon: CloudLightning, color: "darkblue", label: "Extrema" };
    default:
      return { icon: CloudRain, color: "darkblue", label: "--" };
  }
};

// mm aproximados por nível
const getRainMm = (level: number): number => {
  switch (level) {
    case 0: return 0;   // SEM CHUVA
    case 1: return 2;   // GAROA
    case 2: return 5;   // FRACA
    case 3: return 10;  // MODERADA
    case 4: return 20;  // FORTE
    case 5: return 35;  // INTENSA
    case 6: return 48;  // EXTREMA
    default: return 0;
  }
};

const MAX_DATA_POINTS = 30;

export function WeatherDashboard() {
  const [weatherData, setWeatherData] = useState<WeatherData | null>(null);
  const [historicalData, setHistoricalData] = useState<Record<string, ChartData[]>>({
    temperatura: [],
    umidade: [],
    chuva_nivel: [],
    pressao: [],
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

                if (key === "chuva_nivel" || key === "temperatura" || key === "umidade" || key === "pressao") {
                  const dataArray = [...(newHistoricalData[key] || []), newPoint];
                  if (dataArray.length > MAX_DATA_POINTS) dataArray.shift();
                  newHistoricalData[key] = dataArray;
                }
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
    if (!value && value !== 0) return "--";
    return value.toFixed(decimals);
  };

  const rainLevel = weatherData?.chuva_nivel ?? 0;
  const rainMm = getRainMm(rainLevel);

  return (
    <div className="flex flex-col min-h-screen bg-background">
      <main className="flex-grow">
        <div className="container mx-auto px-4 py-8 max-w-7xl">
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

          <div className="weather-grid">
            {(() => {
              const tempConfig = getTempConfig(weatherData?.temperatura ?? 25);
              return (
                <WeatherCard
                  title="Temperatura"
                  value={formatValue(weatherData?.temperatura)}
                  unit="°C"
                  icon={tempConfig.icon}
                  color="temperature"
                  isLoading={isLoading}
                  customColor={tempConfig.color}
                  statusText={tempConfig.label}
                >
                  <WeatherChart
                    data={historicalData.temperatura}
                    dataKey="value"
                    unit=""
                    stroke={tempConfig.color}
                    title="Temperatura em Tempo Real (°C)"
                  />
                </WeatherCard>
              );
            })()}

            {(() => {
              const humConfig = getHumidityConfig(weatherData?.umidade ?? 50);
              return (
                <WeatherCard
                  title="Umidade"
                  value={formatValue(weatherData?.umidade, 0)}
                  unit="%"
                  icon={humConfig.icon}
                  color="humidity"
                  isLoading={isLoading}
                  customColor={humConfig.color}
                  statusText={humConfig.label}
                >
                  <WeatherChart
                    data={historicalData.umidade}
                    dataKey="value"
                    unit=""
                    stroke={humConfig.color}
                    title="Umidade em Tempo Real (%)"
                  />
                </WeatherCard>
              );
            })()}

            {(() => {
              const pressConfig = getPressureConfig(weatherData?.pressao ?? 1013);
              return (
                <WeatherCard
                  title="Pressão"
                  value={formatValue(weatherData?.pressao, 0)}
                  unit="hPa"
                  icon={pressConfig.icon}
                  color="pressure"
                  isLoading={isLoading}
                  customColor={pressConfig.color}
                  statusText={pressConfig.label}
                >
                  <WeatherChart
                    data={historicalData.pressao}
                    dataKey="value"
                    unit=""
                    stroke={pressConfig.color}
                    title="Pressão em Tempo Real (hPa)"
                  />
                </WeatherCard>
              );
            })()}

            {(() => {
              const rainConfig = getRainConfig(rainLevel);
              const intensidadeTexto = weatherData?.chuva_intensidade || "Sem dados";
              const mmTexto = rainMm > 0 ? `${rainMm} mm` : "0 mm";

              return (
                <WeatherCard
                  title="Precipitação"
                  value={`${intensidadeTexto}" mm"`}
                  unit=""
                  icon={rainConfig.icon}
                  color="precipitation"
                  isLoading={isLoading}
                  customColor={rainConfig.color}
                  statusText={`"Equivalente a "${mmTexto}`}
                >
                  <WeatherChart
                    data={historicalData.chuva_nivel}
                    dataKey="value"
                    unit=""
                    stroke={rainConfig.color}
                    title="Nível de Intensidade"
                    yDomain={[0, 6]}
                    yTicks={[0, 1, 2, 3, 4, 5, 6]}
                  />
                </WeatherCard>
              );
            })()}
          </div>
        </div>
      </main>

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

