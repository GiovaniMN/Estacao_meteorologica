import fetch from 'node-fetch';

// Configuração extraída do estacao.ino
const API_KEY = "AIzaSyBRk9NsauMiHqr9WOcLf6pfIoufUeGAF18";
const DATABASE_URL = "https://estacao-meteorologica-6e2a7-default-rtdb.firebaseio.com";
const USER_EMAIL = "acesso@gmail.com";
const USER_PASSWORD = "acesso123";

// Categorias de chuva
const RAIN_CATEGORIES = [
    { nivel: 0, intensidade: "SEM CHUVA", descricao: "Sem precipitacao" },
    { nivel: 1, intensidade: "GAROA", descricao: "Chuva muito fraca" },
    { nivel: 2, intensidade: "FRACA", descricao: "Chuva fraca" },
    { nivel: 3, intensidade: "MODERADA", descricao: "Chuva moderada" },
    { nivel: 4, intensidade: "FORTE", descricao: "Chuva forte" },
    { nivel: 5, intensidade: "INTENSA", descricao: "Chuva muito forte" },
    { nivel: 6, intensidade: "EXTREMA", descricao: "Temporal/Enchente" }
];

async function authenticate() {
    const url = `https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=${API_KEY}`;
    const response = await fetch(url, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
            email: USER_EMAIL,
            password: USER_PASSWORD,
            returnSecureToken: true
        })
    });
    const data = await response.json();
    if (data.error) throw new Error(data.error.message);
    return data.idToken;
}

async function updateWeather(token, scenario) {
    const url = `${DATABASE_URL}/sensores.json?auth=${token}`;

    // Gera valores aleatórios baseados no cenário
    let temp, humidity, pressure, rain;

    // Verifica se o cenário é um número (nível específico)
    const level = parseInt(scenario);
    if (!isNaN(level) && level >= 0 && level <= 6) {
        // Cenário personalizado por nível
        temp = 20 + Math.random() * 10;
        humidity = 50 + Math.random() * 40;
        pressure = 1000 + Math.random() * 20;
        rain = RAIN_CATEGORIES[level];
    } else {
        // Cenários predefinidos
        switch (scenario) {
            case 'sunny':
                temp = 28 + Math.random() * 5;
                humidity = 30 + Math.random() * 20;
                pressure = 1015 + Math.random() * 10;
                rain = RAIN_CATEGORIES[0];
                break;
            case 'rainy':
                temp = 18 + Math.random() * 5;
                humidity = 80 + Math.random() * 15;
                pressure = 1000 + Math.random() * 10;
                rain = RAIN_CATEGORIES[Math.floor(Math.random() * 3) + 2]; // 2-4
                break;
            case 'storm':
                temp = 22 + Math.random() * 5;
                humidity = 90 + Math.random() * 10;
                pressure = 990 + Math.random() * 10;
                rain = RAIN_CATEGORIES[Math.floor(Math.random() * 2) + 5]; // 5-6
                break;
            default: // random
                temp = 15 + Math.random() * 20;
                humidity = 40 + Math.random() * 50;
                pressure = 990 + Math.random() * 40;
                rain = RAIN_CATEGORIES[Math.floor(Math.random() * 7)];
        }
    }

    const data = {
        temperatura: parseFloat(temp.toFixed(1)),
        umidade: parseFloat(humidity.toFixed(1)),
        pressao: parseFloat(pressure.toFixed(1)),
        chuva_nivel: rain.nivel,
        chuva_intensidade: rain.intensidade,
        chuva_descricao: rain.descricao,
        timestamp: { ".sv": "timestamp" }
    };

    const response = await fetch(url, {
        method: 'PATCH',
        body: JSON.stringify(data)
    });

    if (!response.ok) throw new Error(await response.text());

    console.log(`\nAtualizado para cenário: ${!isNaN(level) ? 'NÍVEL ' + level : scenario.toUpperCase()}`);
    console.log(`Temperatura: ${data.temperatura}°C`);
    console.log(`Umidade: ${data.umidade}%`);
    console.log(`Chuva: ${data.chuva_intensidade} (Nível ${data.chuva_nivel})`);
}

async function main() {
    try {
        console.log("Autenticando...");
        const token = await authenticate();
        console.log("Autenticado com sucesso!");

        const args = process.argv.slice(2);
        const scenario = args[0] || 'random';

        await updateWeather(token, scenario);

    } catch (error) {
        console.error("Erro:", error.message);
    }
}

main();
