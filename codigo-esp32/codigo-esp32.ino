#include <WiFi.h>
#include <WebServer.h>

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
// CONFIGURAÇÃO DO SENSOR
// =========================
#define DISTANCIA_MINIMA 30

// =========================
// WIFI
// =========================
const char* ssid = "ESP32_RC";
const char* password = "esp32pass";

WebServer server(80);

// =========================
// VARIÁVEIS
// =========================
bool andandoFrente = false;

// =========================
// SENSOR ULTRASSÔNICO
// =========================

float medirDistancia() {

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duracao = pulseIn(ECHO_PIN, HIGH, 30000);

  // Se não recebeu resposta
  if (duracao == 0) {
    return 999;
  }

  float distancia = duracao * 0.0343 / 2;

  return distancia;
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

  Serial.print("Distancia: ");
  Serial.print(distancia);
  Serial.println(" cm");

  // Obstáculo muito próximo
  if (distancia <= DISTANCIA_MINIMA) {

    Serial.println("OBSTACULO DETECTADO! PARANDO!");

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
// COMANDO
// =========================

void handleCommand() {

  if (!server.hasArg("cmd")) {

    server.send(
      400,
      "text/plain",
      "Comando ausente"
    );

    return;
  }

  String cmd = server.arg("cmd");

  Serial.print("Command received: ");
  Serial.println(cmd);

  if (cmd == "forward") {
    forward();
  }

  else if (cmd == "backward") {
    backward();
  }

  else if (cmd == "left") {
    left();
  }

  else if (cmd == "right") {
    right();
  }

  else if (cmd == "stop") {
    stopMotors();
  }

  server.send(
    200,
    "text/plain",
    "OK"
  );
}

// =========================
// PÁGINA DO CONTROLE
// =========================

const char webpage[] PROGMEM = R"rawliteral(

<!DOCTYPE html>

<html lang="pt-BR">

<head>

<meta charset="UTF-8">

<meta name="viewport"
      content="width=device-width,
               initial-scale=1.0,
               maximum-scale=1.0,
               user-scalable=no">

<title>ESP32 RC CAR</title>

<style>

* {
  box-sizing: border-box;
  -webkit-tap-highlight-color: transparent;
}

body {

  margin: 0;

  background: #111;

  color: white;

  font-family: Arial, sans-serif;

  text-align: center;

  user-select: none;

  touch-action: manipulation;
}

h1 {

  margin-top: 25px;

  font-size: 30px;
}

.controls {

  width: 300px;

  margin: 35px auto;

  display: grid;

  grid-template-columns: 90px 90px 90px;

  grid-template-rows: 90px 90px 90px;

  gap: 15px;

  justify-content: center;
}

button {

  width: 90px;

  height: 90px;

  font-size: 42px;

  border: none;

  border-radius: 20px;

  background: #333;

  color: white;

  cursor: pointer;

  touch-action: manipulation;
}

button:active {

  transform: scale(0.95);
}

.forward {

  grid-column: 2;

  grid-row: 1;
}

.left {

  grid-column: 1;

  grid-row: 2;
}

.stop {

  grid-column: 2;

  grid-row: 2;

  background: #b00000;
}

.right {

  grid-column: 3;

  grid-row: 2;
}

.backward {

  grid-column: 2;

  grid-row: 3;
}

</style>

</head>

<body>

<h1>ESP32 RC CAR</h1>

<div class="controls">

  <button class="forward"
          onclick="sendCommand('forward')">

    &#8593;

  </button>

  <button class="left"
          onclick="sendCommand('left')">

    &#8592;

  </button>

  <button class="stop"
          onclick="sendCommand('stop')">

    &#9632;

  </button>

  <button class="right"
          onclick="sendCommand('right')">

    &#8594;

  </button>

  <button class="backward"
          onclick="sendCommand('backward')">

    &#8595;

  </button>

</div>

<script>

function sendCommand(command) {

  fetch('/command?cmd=' + command)

    .then(response => response.text())

    .then(data => console.log(data))

    .catch(error => console.log(error));

}

</script>

</body>

</html>

)rawliteral";

// =========================
// SETUP
// =========================

void setup() {

  Serial.begin(115200);

  // Motores

  pinMode(ENA_PIN, OUTPUT);

  pinMode(IN1_PIN, OUTPUT);

  pinMode(IN2_PIN, OUTPUT);

  pinMode(ENB_PIN, OUTPUT);

  pinMode(IN3_PIN, OUTPUT);

  pinMode(IN4_PIN, OUTPUT);

  // Sensor

  pinMode(TRIG_PIN, OUTPUT);

  pinMode(ECHO_PIN, INPUT);

  digitalWrite(TRIG_PIN, LOW);

  // Para motores ao iniciar

  stopMotors();

  // Wi-Fi

  WiFi.softAP(
    ssid,
    password
  );

  Serial.println();

  Serial.println("ESP32 RC CAR");

  Serial.print("Access Point: ");

  Serial.println(ssid);

  Serial.print("AP IP address: ");

  Serial.println(WiFi.softAPIP());

  // Página

  server.on("/", []() {

    server.send_P(

      200,

      "text/html; charset=UTF-8",

      webpage

    );

  });

  // Comandos

  server.on(
    "/command",
    handleCommand
  );

  server.begin();

  Serial.println("Web server started");

}

// =========================
// LOOP
// =========================

void loop() {

  server.handleClient();

  // =========================
  // VERIFICA OBSTÁCULO
  // ENQUANTO ESTÁ INDO PARA FRENTE
  // =========================

  if (andandoFrente) {

    float distancia = medirDistancia();

    Serial.print("Distancia: ");

    Serial.print(distancia);

    Serial.println(" cm");

    if (distancia <= DISTANCIA_MINIMA) {

      Serial.println(
        "OBSTACULO DETECTADO! PARANDO!"
      );

      stopMotors();

    }

    delay(100);
  }
}