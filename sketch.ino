#include <WiFi.h>
#include <PubSubClient.h>
#include "DHTesp.h"


const char* BROKER = "mqtt3.thingspeak.com";
const int PORTA = 1883;

const char* MQTT_CLIENT_ID = "SEU_ID";
const char* MQTT_USUARIO   = "SEU_USER";
const char* MQTT_SENHA     = "SUA_SENHA";

const unsigned long CANAL_ID = 3569917;
const int LIMITE = 28;

const int PINO_DHT = 15;
const int PINO_LED = 2;
const int PINO_BUZZER = 4;

DHTesp dht;
WiFiClient wifi;
PubSubClient mqtt(wifi);

bool alarme = false;
bool existeLeituraValida = false;
bool falhaSensorPendente = false;
bool falhaSensorJaDetectada = false;

int falhasConsecutivas = 0;

TempAndHumidity ultimaLeituraValida;

unsigned long ultimaLeitura = 0;
unsigned long ultimaPublicacao = 0;
unsigned long ultimaTentativa = 0;

const unsigned long INTERVALO_LEITURA_MS = 2000;
const unsigned long INTERVALO_PUBLICACAO_MS = 20000;
const int MAX_FALHAS_SENSOR = 3;

bool decidirAlarme(float temperatura, bool alarmeAtual) {
  if (!alarmeAtual && temperatura > LIMITE) {
    return true;
  }

  if (alarmeAtual && temperatura < LIMITE - 1) {
    return false;
  }

  return alarmeAtual;
}

bool conectarMqtt() {
  if (!mqtt.connect(MQTT_CLIENT_ID, MQTT_USUARIO, MQTT_SENHA)) {
    Serial.printf("Falha ao conectar ao MQTT. Estado: %d\n", mqtt.state());
    return false;
  }

  Serial.println("Conectado ao ThingSpeak via MQTT.");
  return true;
}

bool publicarNoThingSpeak(float temperatura,
                          float umidade,
                          bool alarmeLigado,
                          bool falhaSensor) {
  char topico[64];
  char dados[128];

  snprintf(topico, sizeof(topico),
           "channels/%lu/publish",
           (unsigned long)CANAL_ID);

  if (falhaSensor) {
    snprintf(dados, sizeof(dados),
             "status=FALHA_SENSOR");
  } else if (alarmeLigado) {
    snprintf(dados, sizeof(dados),
             "field1=%.1f&field2=%.0f&status=ALERTA",
             temperatura, umidade);
  } else {
    snprintf(dados, sizeof(dados),
             "field1=%.1f&field2=%.0f",
             temperatura, umidade);
  }

  bool enviado = mqtt.publish(topico, dados);

  Serial.printf("%s: %s\n",
                enviado ? "Publicação enviada" : "Falha na publicação",
                dados);

  return enviado;
}

void setup() {
  Serial.begin(115200);

  pinMode(PINO_LED, OUTPUT);
  pinMode(PINO_BUZZER, OUTPUT);
  digitalWrite(PINO_LED, LOW);

  dht.setup(PINO_DHT, DHTesp::DHT22);

  Serial.printf("Monitor do depósito | canal=%lu | limite=%d C\n",
                (unsigned long)CANAL_ID, LIMITE);

  Serial.print("Conectando ao Wi-Fi");
  WiFi.begin("Wokwi-GUEST", "", 6);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  Serial.println(" conectado!");

  mqtt.setServer(BROKER, PORTA);
  mqtt.setBufferSize(512);
  mqtt.setKeepAlive(60);
}

void loop() {
  if (!mqtt.connected() &&
      millis() - ultimaTentativa >= 3000) {
    ultimaTentativa = millis();

    Serial.print("Conectando ao ThingSpeak... ");
    if (conectarMqtt()) {
      Serial.println("ok");
    }
  }

  mqtt.loop();

  if (millis() - ultimaLeitura >= INTERVALO_LEITURA_MS) {
    ultimaLeitura = millis();

    TempAndHumidity leitura = dht.getTempAndHumidity();

    if (dht.getStatus() == DHTesp::ERROR_NONE) {
      ultimaLeituraValida = leitura;
      existeLeituraValida = true;
      falhasConsecutivas = 0;
      falhaSensorJaDetectada = false;

      bool alarmeAnterior = alarme;
      alarme = decidirAlarme(leitura.temperature, alarmeAnterior);

      digitalWrite(PINO_LED, alarme ? HIGH : LOW);

      if (!alarmeAnterior && alarme) {
        tone(PINO_BUZZER, 1000, 500);
        Serial.println("Alarme de temperatura ativado.");
      }

      Serial.printf("Leitura: %.1f C, %.0f %% | alarme=%s\n",
                    leitura.temperature,
                    leitura.humidity,
                    alarme ? "LIGADO" : "desligado");
    } else {
      falhasConsecutivas++;

      Serial.printf("Leitura inválida do DHT22. Falhas consecutivas: %d\n",
                    falhasConsecutivas);

      if (falhasConsecutivas >= MAX_FALHAS_SENSOR &&
          !falhaSensorJaDetectada) {
        falhaSensorPendente = true;
        falhaSensorJaDetectada = true;
        Serial.println("Falha do sensor detectada; alerta pendente.");
      }
    }
  }

  if (millis() - ultimaPublicacao >= INTERVALO_PUBLICACAO_MS) {
    ultimaPublicacao = millis();

    if (mqtt.connected()) {
      if (falhaSensorPendente) {
        bool enviado = publicarNoThingSpeak(0, 0, false, true);

        if (enviado) {
          falhaSensorPendente = false;
        }
      } else if (existeLeituraValida) {
        publicarNoThingSpeak(
          ultimaLeituraValida.temperature,
          ultimaLeituraValida.humidity,
          alarme,
          false
        );
      }
    }
  }
}