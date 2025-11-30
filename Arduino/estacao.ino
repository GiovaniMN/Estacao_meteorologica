#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <LiquidCrystal_I2C.h>

// ===== CONFIG WIFI =====
#define WIFI_SSID "AdrianoA31"
#define WIFI_PASSWORD "snkj3219"

// ===== CONFIG FIREBASE =====
#define API_KEY "AIzaSyBRk9NsauMiHqr9WOcLf6pfIoufUeGAF18"
#define DATABASE_URL "estacao-meteorologica-6e2a7-default-rtdb.firebaseio.com"
#define USER_EMAIL "acesso@gmail.com"
#define USER_PASSWORD "acesso123"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ===== CONFIG BME280 =====
Adafruit_BME280 bme;

// ===== SENSOR DE ÁGUA / CHUVA =====
#define WATER_PWR_PIN 25
#define WATER_ADC_PIN 35
#define POWER_ON_DELAY 150
#define READING_DELAY 20

// ===== CATEGORIAS DE INTENSIDADE DE CHUVA =====
struct IntensidadeCategoria {
  const char* nome;
  const char* nome_curto;  // Para LCD (máx 4 caracteres)
  const char* descricao;
  int adc_min;
  int adc_max;
  int nivel;  // 0-6
};

const int NUM_CATEGORIAS = 7;
const IntensidadeCategoria categorias[NUM_CATEGORIAS] = {
  {"SEM CHUVA", "Seco", "Sem precipitacao",     0, 1400, 0},
  {"GAROA",     "Garoa", "Chuva muito fraca", 1400, 1700, 1},
  {"FRACA",     "Fraca", "Chuva fraca",       1700, 2000, 2},
  {"MODERADA",  "Media", "Chuva moderada",    2000, 2500, 3},
  {"FORTE",     "Forte", "Chuva forte",       2500, 3000, 4},
  {"INTENSA",   "Intensa", "Chuva muito forte", 3000, 3700, 5},
  {"EXTREMA",   "Extrema", "Temporal/Enchente", 3700, 4095, 6}
};

// ===== CONFIG LCD =====
#define I2C_ADDR 0x27
#define LCD_COLUMNS 16
#define LCD_ROWS 2
LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLUMNS, LCD_ROWS);

// Timer
unsigned long lastTime = 0;
const long timerDelay = 2500;  // 2.5 segundos

// Variáveis de estado
int categoria_atual = 0;
int adc_atual = 0;
unsigned long tempo_na_categoria = 0;
unsigned long tempo_inicio_categoria = 0;

// Histórico de tempo em cada categoria (em segundos)
unsigned long tempo_total_categoria[NUM_CATEGORIAS] = {0};

// ===== FUNÇÕES AUXILIARES =====

void setupWaterSensor() {
  pinMode(WATER_PWR_PIN, OUTPUT);
  digitalWrite(WATER_PWR_PIN, LOW);
  
  analogReadResolution(12);
  analogSetPinAttenuation(WATER_ADC_PIN, ADC_6db);
  
  Serial.println("✓ Sensor configurado");
  Serial.println("  Modo: Classificação por Intensidade");
}

int readADCstable(int pin, int samples = 15) {
  int readings[samples];
  
  for (int i = 0; i < samples; i++) {
    readings[i] = analogRead(pin);
    delay(READING_DELAY);
  }
  
  // Ordenação
  for (int i = 0; i < samples - 1; i++) {
    for (int j = i + 1; j < samples; j++) {
      if (readings[i] > readings[j]) {
        int temp = readings[i];
        readings[i] = readings[j];
        readings[j] = temp;
      }
    }
  }
  
  // Descarta extremos e calcula média
  int discard = samples / 5;
  if (discard < 1) discard = 1;
  
  long sum = 0;
  int count = 0;
  
  for (int i = discard; i < samples - discard; i++) {
    sum += readings[i];
    count++;
  }
  
  return (count > 0) ? (sum / count) : readings[samples / 2];
}

// Classifica ADC em categoria de intensidade
int classificarIntensidade(int adc) {
  for (int i = 0; i < NUM_CATEGORIAS; i++) {
    if (adc >= categorias[i].adc_min && adc <= categorias[i].adc_max) {
      return i;
    }
  }
  return 0;  // Padrão: SEM CHUVA
}

// Converte segundos para formato legível
String formataTempo(unsigned long segundos) {
  if (segundos < 60) {
    return String(segundos) + "s";
  } else if (segundos < 3600) {
    int min = segundos / 60;
    int seg = segundos % 60;
    return String(min) + "m" + String(seg) + "s";
  } else {
    int hrs = segundos / 3600;
    int min = (segundos % 3600) / 60;
    return String(hrs) + "h" + String(min) + "m";
  }
}

// -------- SETUP --------
void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println("\n╔═════════════════════════════════════════╗");
  Serial.println("║  Estação Meteorológica v5.2            ║");
  Serial.println("║  Display Otimizado 16x2                ║");
  Serial.println("╚═════════════════════════════════════════╝\n");

  // WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\n✓ WiFi conectado!");

  // Sensores
  setupWaterSensor();

  if (!bme.begin(0x76)) {
    Serial.println("❌ BME280 não encontrado!");
    while (1) delay(100);
  }
  Serial.println("✓ BME280 inicializado");

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  Estacao");
  lcd.setCursor(0, 1);
  lcd.print(" Meteorologica");
  Serial.println("✓ LCD inicializado");

  // Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("✓ Firebase configurado");

  Serial.println("\n═══ Categorias de Intensidade ═══");
  for (int i = 0; i < NUM_CATEGORIAS; i++) {
    Serial.printf("[%d] %-12s: ADC %4d-%4d | LCD: %s\n", 
                  categorias[i].nivel,
                  categorias[i].nome,
                  categorias[i].adc_min,
                  categorias[i].adc_max,
                  categorias[i].nome_curto);
  }
  
  Serial.println("\n════ Sistema Pronto! ════\n");
  tempo_inicio_categoria = millis();
  delay(2000);
}

// -------- LOOP --------
void loop() {
  unsigned long currentTime = millis();
  
  if ((currentTime - lastTime) > timerDelay) {
    
    // ==== Leitura BME280 ====
    float temperatura = bme.readTemperature();
    float umidade = bme.readHumidity();
    float pressao = bme.readPressure() / 100.0F;  // Converte para hPa

    // ==== Leitura sensor de água ====
    digitalWrite(WATER_PWR_PIN, HIGH);
    delay(POWER_ON_DELAY);
    
    adc_atual = readADCstable(WATER_ADC_PIN, 15);
    
    digitalWrite(WATER_PWR_PIN, LOW);

    // ==== Classificação de Intensidade ====
    int nova_categoria = classificarIntensidade(adc_atual);
    
    // Atualiza tempo na categoria
    if (nova_categoria != categoria_atual) {
      // Mudou de categoria: salva tempo da categoria anterior
      tempo_na_categoria = (currentTime - tempo_inicio_categoria) / 1000;
      tempo_total_categoria[categoria_atual] += tempo_na_categoria;
      
      Serial.printf(">>> MUDANÇA: %s → %s (ficou %s)\n",
                    categorias[categoria_atual].nome,
                    categorias[nova_categoria].nome,
                    formataTempo(tempo_na_categoria).c_str());
      
      categoria_atual = nova_categoria;
      tempo_inicio_categoria = currentTime;
    }
    
    tempo_na_categoria = (currentTime - tempo_inicio_categoria) / 1000;

    // ==== Monitor Serial ====
    Serial.println("┌────────────────────────────────────────────────┐");
    Serial.printf("│ INTENSIDADE: %-12s Nível: %d         │\n", 
                  categorias[categoria_atual].nome, 
                  categorias[categoria_atual].nivel);
    Serial.printf("│ ADC: %4d | Tempo nesta: %s%*s│\n", 
                  adc_atual, 
                  formataTempo(tempo_na_categoria).c_str(),
                  14 - formataTempo(tempo_na_categoria).length(), "");
    Serial.println("├────────────────────────────────────────────────┤");
    Serial.printf("│ Temperatura: %5.1f°C                        │\n", temperatura);
    Serial.printf("│ Umidade:     %5.1f%%                        │\n", umidade);
    Serial.printf("│ Pressão:     %7.2f hPa                    │\n", pressao);
    Serial.println("└────────────────────────────────────────────────┘\n");

    // ==== Atualiza LCD - OTIMIZADO PARA 16 CARACTERES ====
    lcd.clear();
    
    // LINHA 1: Temperatura e Umidade
    // Formato: "T:24.5C U:65%   " (16 caracteres)
    lcd.setCursor(0, 0);
    lcd.print("P:");
    lcd.print(pressao, 0);  // Ex: 1013 (4 dígitos)
    lcd.print("hPa ");
    
    lcd.print("U:");
    lcd.print(umidade, 0);  // Ex: 65
    lcd.print("%");
    
    // LINHA 2: Pressão e Intensidade de Chuva
    // Formato: "P:1013 Seco N0  " ou "P:1008 Fort N4  "
    lcd.setCursor(0, 1);
    lcd.print("T:");
    lcd.print(temperatura, 1);  // Ex: 24.5
    lcd.print((char)223);  // Símbolo de grau °
    lcd.print("C ");
    lcd.print(categorias[categoria_atual].nome_curto);  // Ex: "Seco" (4 caracteres)
    
    // Adiciona nível se houver chuva
    if (categorias[categoria_atual].nivel > 0) {
      lcd.print(" N");
      lcd.print(categorias[categoria_atual].nivel);
    }

    // ==== Envia ao Firebase ====
    if (Firebase.ready()) {
      // Clima
      if (!Firebase.RTDB.setFloat(&fbdo, "/sensores/temperatura", temperatura)) {
        Serial.printf("ERRO ao enviar: %s\n", fbdo.errorReason().c_str());
      }
      Firebase.RTDB.setFloat(&fbdo, "/sensores/umidade", umidade);
      Firebase.RTDB.setFloat(&fbdo, "/sensores/pressao", pressao);
      
      // Intensidade de chuva
      Firebase.RTDB.setInt(&fbdo, "/sensores/chuva_adc", adc_atual);
      Firebase.RTDB.setString(&fbdo, "/sensores/chuva_intensidade", categorias[categoria_atual].nome);
      Firebase.RTDB.setString(&fbdo, "/sensores/chuva_descricao", categorias[categoria_atual].descricao);
      Firebase.RTDB.setInt(&fbdo, "/sensores/chuva_nivel", categorias[categoria_atual].nivel);
      Firebase.RTDB.setInt(&fbdo, "/sensores/chuva_tempo_atual_seg", tempo_na_categoria);
      
      // Histórico de tempo por categoria
      for (int i = 0; i < NUM_CATEGORIAS; i++) {
        String path = "/sensores/historico/tempo_" + String(categorias[i].nome);
        Firebase.RTDB.setInt(&fbdo, path.c_str(), tempo_total_categoria[i]);
      }
      
      Firebase.RTDB.setTimestamp(&fbdo, "/sensores/timestamp");
      
      // Alerta para chuva forte
      if (categorias[categoria_atual].nivel >= 4) {
        Firebase.RTDB.setBool(&fbdo, "/sensores/alerta_chuva_forte", true);
      } else {
        Firebase.RTDB.setBool(&fbdo, "/sensores/alerta_chuva_forte", false);
      }
    }

    lastTime = currentTime;
  }
}
