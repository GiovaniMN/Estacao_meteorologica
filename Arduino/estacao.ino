#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <LiquidCrystal_I2C.h>

#define WIFI_SSID "AdrianoA31"
#define WIFI_PASSWORD "snkj3219"

#define API_KEY "AIzaSyBRk9NsauMiHqr9WOcLf6pfIoufUeGAF18"
#define DATABASE_URL "https://estacao-meteorologica-6e2a7-default-rtdb.firebaseio.com/"
#define USER_EMAIL "acesso@gmail.com"
#define USER_PASSWORD "acesso123"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

Adafruit_BME280 bme;

#define WATER_PWR_PIN 25
#define WATER_ADC_PIN 35
#define POWER_ON_DELAY 150
#define READING_DELAY 20

struct IntensidadeCategoria {
  const char* nome;
  const char* nome_curto;
  const char* descricao;
  int adc_min;
  int adc_max;
  int nivel;
};

const int NUM_CATEGORIAS = 7;
const IntensidadeCategoria categorias[NUM_CATEGORIAS] = {
  {"SEM CHUVA", "Seco",   "Sem precipitacao",     0, 300, 0},
  {"GAROA",     "Garoa",  "Chuva muito fraca", 300, 310, 1},
  {"FRACA",     "Fraca",  "Chuva fraca",       310, 320, 2},
  {"MODERADA",  "Media",  "Chuva moderada",    320, 330, 3},
  {"FORTE",     "Forte",  "Chuva forte",       330, 340, 4},
  {"INTENSA",   "Intensa", "Chuva muito forte", 340, 350, 5},
  {"EXTREMA",   "Extrema", "Temporal/Enchente", 350, 360, 6}
};

#define I2C_ADDR 0x27
#define LCD_COLUMNS 16
#define LCD_ROWS 2
LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLUMNS, LCD_ROWS);

unsigned long lastTime = 0;
const long timerDelay = 10000;

int categoria_atual = 0;
int adc_atual = 0;
unsigned long tempo_na_categoria = 0;
unsigned long tempo_inicio_categoria = 0;

unsigned long tempo_total_categoria[NUM_CATEGORIAS] = {0};

void setupWaterSensor() {
  pinMode(WATER_PWR_PIN, OUTPUT);
  digitalWrite(WATER_PWR_PIN, LOW);

  analogReadResolution(12);
  analogSetPinAttenuation(WATER_ADC_PIN, ADC_6db);

  Serial.println("✓ Sensor de agua configurado");
}

int readADCstable(int pin, int samples = 15) {
  int readings[samples];

  for (int i = 0; i < samples; i++) {
    readings[i] = analogRead(pin);
    delay(READING_DELAY);
  }

  for (int i = 0; i < samples - 1; i++) {
    for (int j = i + 1; j < samples; j++) {
      if (readings[i] > readings[j]) {
        int temp = readings[i];
        readings[i] = readings[j];
        readings[j] = temp;
      }
    }
  }

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

int classificarIntensidade(int adc) {
  for (int i = 0; i < NUM_CATEGORIAS; i++) {
    if (adc >= categorias[i].adc_min && adc <= categorias[i].adc_max) {
      return i;
    }
  }
  return 0;
}

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

void reconnectWiFiIfNeeded() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.println("WiFi desconectado, tentando reconectar...");
  WiFi.disconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi reconectado.");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Falha ao reconectar WiFi.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n==== Estacao Meteorologica ESP32 + Firebase ====\n");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\n✓ WiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  setupWaterSensor();

  if (!bme.begin(0x76)) {
    Serial.println("❌ BME280 nao encontrado!");
    while (1) delay(100);
  }
  Serial.println("✓ BME280 inicializado");

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  Estacao");
  lcd.setCursor(0, 1);
  lcd.print(" Meteorologica");
  Serial.println("✓ LCD inicializado");

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.token_status_callback = tokenStatusCallback;

  config.timeout.serverResponse = 10 * 1000;
  fbdo.setBSSLBufferSize(8192, 2048);
  fbdo.setResponseSize(4096);

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("✓ Firebase inicializado");

  Serial.println("\nCategorias de intensidade:");
  for (int i = 0; i < NUM_CATEGORIAS; i++) {
    Serial.printf("  [%d] %-8s  ADC %4d-%4d (LCD: %s)\n",
                  categorias[i].nivel,
                  categorias[i].nome,
                  categorias[i].adc_min,
                  categorias[i].adc_max,
                  categorias[i].nome_curto);
  }

  tempo_inicio_categoria = millis();
  lastTime = millis();
}

void loop() {
  reconnectWiFiIfNeeded();

  unsigned long currentTime = millis();

  if ((currentTime - lastTime) > timerDelay && Firebase.ready()) {

    float temperatura = bme.readTemperature();
    float umidade = bme.readHumidity();
    float pressao = bme.readPressure() / 100.0F;

    digitalWrite(WATER_PWR_PIN, HIGH);
    delay(POWER_ON_DELAY);
    adc_atual = readADCstable(WATER_ADC_PIN, 15);
    digitalWrite(WATER_PWR_PIN, LOW);

    int nova_categoria = classificarIntensidade(adc_atual);

    if (nova_categoria != categoria_atual) {
      tempo_na_categoria = (currentTime - tempo_inicio_categoria) / 1000;
      tempo_total_categoria[categoria_atual] += tempo_na_categoria;

      Serial.printf(">>> MUDANCA: %s -> %s (ficou %s)\n",
                    categorias[categoria_atual].nome,
                    categorias[nova_categoria].nome,
                    formataTempo(tempo_na_categoria).c_str());

      categoria_atual = nova_categoria;
      tempo_inicio_categoria = currentTime;
    }
    tempo_na_categoria = (currentTime - tempo_inicio_categoria) / 1000;

    Serial.printf("Sensor agua ADC=%d | Intensidade=%s (N%d)\n",
                  adc_atual,
                  categorias[categoria_atual].nome,
                  categorias[categoria_atual].nivel);

    Serial.printf("T=%.2fC U=%.1f%% P=%.1fhPa\n",
                  temperatura, umidade, pressao);

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("P:");
    lcd.print(pressao, 0);
    lcd.print("hPa ");
    lcd.print("U:");
    lcd.print(umidade, 0);
    lcd.print("%");

    lcd.setCursor(0, 1);
    lcd.print("T:");
    lcd.print(temperatura, 1);
    lcd.print((char)223);
    lcd.print("C ");
    lcd.print(categorias[categoria_atual].nome_curto);

    if (!Firebase.RTDB.setFloat(&fbdo, "/sensores/temperatura", temperatura)) {
      Serial.printf("ERRO temp: %s\n", fbdo.errorReason().c_str());
    }
    Firebase.RTDB.setFloat(&fbdo, "/sensores/umidade", umidade);
    Firebase.RTDB.setFloat(&fbdo, "/sensores/pressao", pressao);

    Firebase.RTDB.setInt(&fbdo, "/sensores/chuva_adc", adc_atual);
    Firebase.RTDB.setString(&fbdo, "/sensores/chuva_intensidade", categorias[categoria_atual].nome);
    Firebase.RTDB.setInt(&fbdo, "/sensores/chuva_nivel", categorias[categoria_atual].nivel);

    Firebase.RTDB.setTimestamp(&fbdo, "/sensores/timestamp");

    lastTime = currentTime;
  }
}
