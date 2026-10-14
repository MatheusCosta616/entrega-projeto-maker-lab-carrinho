#include <WiFi.h>
#include <WebServer.h>
#include <CP2_TinyML_ESP32_inferencing.h>
#include <cstring>

// =========================
// PINOS
// =========================
#define ENA_PIN 13
#define IN1_PIN 14
#define IN2_PIN 12
#define ENB_PIN 25
#define IN3_PIN 27
#define IN4_PIN 26
#define TRIG_PIN 5
#define ECHO_PIN 2

// =========================
// CONFIGURACAO
// =========================
#define DISTANCIA_MINIMA 30.0f
#define DISTANCIA_ATENCAO 50.0f
#define CONFIANCA_MINIMA 0.60f

const char* ssid = "ESP32_RC";
const char* password = "esp32pass";

WebServer server(80);

bool andandoFrente = false;
bool controleManual = false;
bool janelaPronta = false;

float amostrasTinyML[EI_CLASSIFIER_RAW_SAMPLE_COUNT];
size_t indiceAmostra = 0;
unsigned long ultimaAmostraML = 0;

String classeTinyML = "aguardando";
float confiancaTinyML = 0.0f;

// =========================
// SENSOR HC-SR04
// =========================
float medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duracao = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duracao == 0) return -1.0f;

  return duracao * 0.0343f / 2.0f;
}

// =========================
// MOTORES
// =========================
void stopMotors() {
  digitalWrite(ENA_PIN, LOW);
  digitalWrite(ENB_PIN, LOW);
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(IN3_PIN, LOW);
  digitalWrite(IN4_PIN, LOW);
  andandoFrente = false;
  Serial.println("STOP");
}

void forward() {
  float distancia = medirDistancia();

  if (distancia < 0 || distancia <= DISTANCIA_MINIMA) {
    Serial.println("Parada de seguranca: distancia invalida ou obstaculo proximo");
    stopMotors();
    return;
  }

  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(IN3_PIN, HIGH);
  digitalWrite(IN4_PIN, LOW);
  digitalWrite(ENA_PIN, HIGH);
  digitalWrite(ENB_PIN, HIGH);
  andandoFrente = true;
  Serial.println("FORWARD");
}

void backward() {
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, HIGH);
  digitalWrite(IN3_PIN, LOW);
  digitalWrite(IN4_PIN, HIGH);
  digitalWrite(ENA_PIN, HIGH);
  digitalWrite(ENB_PIN, HIGH);
  andandoFrente = false;
  Serial.println("BACKWARD");
}

void left() {
  digitalWrite(ENA_PIN, LOW);
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(IN3_PIN, HIGH);
  digitalWrite(IN4_PIN, LOW);
  digitalWrite(ENB_PIN, HIGH);
  andandoFrente = false;
  Serial.println("LEFT");
}

void right() {
  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(ENA_PIN, HIGH);
  digitalWrite(ENB_PIN, LOW);
  digitalWrite(IN3_PIN, LOW);
  digitalWrite(IN4_PIN, LOW);
  andandoFrente = false;
  Serial.println("RIGHT");
}

// =========================
// EDGE IMPULSE
// =========================
int obterDadosTinyML(size_t offset, size_t length, float *saida) {
  if (offset + length > EI_CLASSIFIER_RAW_SAMPLE_COUNT) {
    return -1;
  }

  memcpy(saida, amostrasTinyML + offset, length * sizeof(float));
  return 0;
}

bool classificarTinyML() {
  signal_t sinal;
  sinal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
  sinal.get_data = obterDadosTinyML;

  ei_impulse_result_t resultado = {0};

  EI_IMPULSE_ERROR erro = run_classifier(&sinal, &resultado, false);
  if (erro != EI_IMPULSE_OK) {
    Serial.printf("Erro TinyML: %d\n", erro);
    classeTinyML = "erro";
    return false;
  }

  confiancaTinyML = 0.0f;
  classeTinyML = "indefinido";

  for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    float valor = resultado.classification[i].value;
    Serial.printf("%s: %.3f\n",
      resultado.classification[i].label, valor);

    if (valor > confiancaTinyML) {
      confiancaTinyML = valor;
      classeTinyML = resultado.classification[i].label;
    }
  }

  Serial.printf("Classe: %s (%.2f)\n",
    classeTinyML.c_str(), confiancaTinyML);

  return true;
}

void atualizarTinyML() {
  if (millis() - ultimaAmostraML < EI_CLASSIFIER_INTERVAL_MS) {
    return;
  }
  ultimaAmostraML = millis();

  float distancia = medirDistancia();

  // Sensor invalido nao entra no modelo
  if (distancia < 0) {
    if (andandoFrente) stopMotors();
    indiceAmostra = 0;
    janelaPronta = false;
    Serial.println("Leitura invalida do sensor");
    return;
  }

  // Protecao independente do modelo
  if (andandoFrente && distancia <= DISTANCIA_MINIMA) {
    Serial.println("OBSTACULO: parada de seguranca");
    stopMotors();
  }

  amostrasTinyML[indiceAmostra++] = distancia;

  if (indiceAmostra >= EI_CLASSIFIER_RAW_SAMPLE_COUNT) {
    indiceAmostra = 0;
    janelaPronta = true;

    if (classificarTinyML() && andandoFrente) {
      if (confiancaTinyML < CONFIANCA_MINIMA ||
          classeTinyML == "perigo") {
        stopMotors();
      } else if (classeTinyML == "atencao") {
        // Sem controle PWM configurado, parar e mais seguro
        stopMotors();
      } else if (classeTinyML != "livre") {
        stopMotors();
      }
    }
  }
}

// =========================
// PAGINA WEB
// =========================
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>ESP32 RC CAR - TinyML</title>
<style>
body{margin:0;background:#111;color:white;font-family:Arial;text-align:center}
h1{margin-top:25px;font-size:28px}
.controls{width:300px;margin:30px auto;display:grid;grid-template-columns:90px 90px 90px;grid-template-rows:90px 90px 90px;gap:15px;justify-content:center}
button{width:90px;height:90px;font-size:42px;border:0;border-radius:20px;background:#333;color:white;touch-action:manipulation}
.forward{grid-column:2;grid-row:1}.left{grid-column:1;grid-row:2}.stop{grid-column:2;grid-row:2;background:#b00000}.right{grid-column:3;grid-row:2}.backward{grid-column:2;grid-row:3}
#status{padding:12px;color:#ddd}
</style>
</head>
<body>
<h1>ESP32 RC CAR</h1>
<div class="controls">
<button class="forward" onclick="sendCommand('forward')">&#8593;</button>
<button class="left" onclick="sendCommand('left')">&#8592;</button>
<button class="stop" onclick="sendCommand('stop')">&#9632;</button>
<button class="right" onclick="sendCommand('right')">&#8594;</button>
<button class="backward" onclick="sendCommand('backward')">&#8595;</button>
</div>
<div id="status">TinyML ativo no ESP32</div>
<script>
function sendCommand(command){
 fetch('/command?cmd='+command)
 .then(r=>r.text()).then(t=>document.getElementById('status').textContent=t)
 .catch(()=>document.getElementById('status').textContent='Falha na comunicacao');
}
</script>
</body>
</html>
)rawliteral";

// =========================
// COMANDOS WEB
// =========================
void handleCommand() {
  if (!server.hasArg("cmd")) {
    server.send(400, "text/plain", "Comando ausente");
    return;
  }

  String cmd = server.arg("cmd");

  if (cmd == "forward") {
    controleManual = true;
    forward();
  } else if (cmd == "backward") {
    controleManual = true;
    backward();
  } else if (cmd == "left") {
    controleManual = true;
    left();
  } else if (cmd == "right") {
    controleManual = true;
    right();
  } else if (cmd == "stop") {
    controleManual = true;
    stopMotors();
  } else {
    server.send(400, "text/plain", "Comando desconhecido");
    return;
  }

  server.send(200, "text/plain",
    "OK | TinyML: " + classeTinyML);
}

// =========================
// SETUP
// =========================
void setup() {
  Serial.begin(115200);

  pinMode(ENA_PIN, OUTPUT);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(ENB_PIN, OUTPUT);
  pinMode(IN3_PIN, OUTPUT);
  pinMode(IN4_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  stopMotors();

  WiFi.softAP(ssid, password);

  Serial.println();
  Serial.println("CP2 TinyML - ESP32 RC CAR");
  Serial.print("Wi-Fi: ");
  Serial.println(ssid);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", []() {
    server.send_P(200, "text/html; charset=UTF-8", webpage);
  });
  server.on("/command", handleCommand);
  server.begin();

  Serial.println("Servidor web iniciado");
  Serial.println("TinyML precisa de 20 leituras para classificar.");
}

// =========================
// LOOP
// =========================
void loop() {
  server.handleClient();
  atualizarTinyML();
  delay(1);
}

