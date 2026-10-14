#include <CP2_TinyML_ESP32_inferencing.h>


/*
  CP2 - TinyML
  Projeto: Carrinho Robo ESP32
  Sensor: HC-SR04
  Objetivo: Coletar dados reais e gerar CSV
  Pinos: TRIG = GPIO 5 | ECHO = GPIO 2

  Comando pelo Monitor Serial:
  c = coletar uma amostra
*/

#define TRIG_PIN 5
#define ECHO_PIN 2

const unsigned long INTERVALO_MS = 100;
const int NUM_LEITURAS = 21;
const unsigned long TIMEOUT_ECHO_US = 30000;

// Mede a distancia usando o HC-SR04
float medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duracao = pulseIn(
    ECHO_PIN,
    HIGH,
    TIMEOUT_ECHO_US
  );

  // -1 significa que nao houve eco valido
  if (duracao == 0) {
    return -1.0;
  }

  float distancia = duracao * 0.0343 / 2.0;

  return distancia;
}

// Coleta 21 leituras e apresenta os dados em CSV
void recolherAmostra() {
  Serial.println();
  Serial.println("timestamp,distance_cm");

  unsigned long inicio = millis();
  unsigned long proximaLeitura = inicio;

  for (int i = 0; i < NUM_LEITURAS; i++) {

    while ((long)(millis() - proximaLeitura) < 0) {
      delay(1);
    }

    float distancia = medirDistancia();
    unsigned long timestamp = millis() - inicio;

    Serial.print(timestamp);
    Serial.print(",");

    if (distancia < 0) {
      Serial.println("-1");
    } else {
      Serial.println(distancia, 2);
    }

    proximaLeitura += INTERVALO_MS;
  }

  Serial.println();
  Serial.println("Amostra concluida.");
  Serial.println("Envia c para coletar outra.");
}

// Configuracao inicial
void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  digitalWrite(TRIG_PIN, LOW);

  Serial.println();
  Serial.println("================================");
  Serial.println(" CP2 - COLETA DE DADOS HC-SR04");
  Serial.println(" ESP32 DevKit");
  Serial.println("================================");
  Serial.println("Envia c para coletar uma amostra.");
}

// Aguarda comandos pelo Monitor Serial
void loop() {
  if (Serial.available() > 0) {

    char comando = Serial.read();

    if (comando == 'c' || comando == 'C') {
      recolherAmostra();
    }
  }
}