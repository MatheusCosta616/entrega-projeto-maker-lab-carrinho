/*
  Robô 2WD com ESP32 + TB6612FNG + HC-SR04

  Controle:
    1) Celular via Wi-Fi: conecte-se à rede "Robo-ESP32" e abra http://192.168.4.1
    2) Wokwi / Serial Monitor:
       F = frente
       B = ré
       L = esquerda
       R = direita
       S = parar
       + = aumenta velocidade
       - = diminui velocidade

  Segurança por ultrassom:
    - Se o HC-SR04 detectar obstáculo a 20 cm ou menos, o robô para.
    - Enquanto houver obstáculo à frente, somente RÉ (B) e STOP (S) são aceitos.
    - O bloqueio é liberado quando a distância volta a 25 cm ou mais.

  Baseado no exemplo de Wi-Fi Scan do ESP32 fornecido para o projeto.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// ============================================================
// Configuração Wi-Fi
// ============================================================
const char* AP_SSID = "Robo-ESP32";
const char* AP_PASSWORD = "fiap2026";

IPAddress AP_IP(192, 168, 4, 1);
IPAddress AP_GATEWAY(192, 168, 4, 1);
IPAddress AP_SUBNET(255, 255, 255, 0);

WebServer server(80);

// ============================================================
// Pinos ESP32 -> TB6612FNG
// ============================================================
// Motor esquerdo - canal A
const uint8_t AIN1_PIN = 25;
const uint8_t AIN2_PIN = 26;
const uint8_t PWMA_PIN = 27;

// Motor direito - canal B
const uint8_t BIN1_PIN = 32;
const uint8_t BIN2_PIN = 33;
const uint8_t PWMB_PIN = 14;

// Standby do driver
const uint8_t STBY_PIN = 13;

// ============================================================
// Pinos ESP32 -> HC-SR04
// ============================================================
const uint8_t ULTRASONIC_TRIG_PIN = 18;
const uint8_t ULTRASONIC_ECHO_PIN = 19;

// Distâncias do bloqueio com histerese para evitar liga/desliga rápido.
const float OBSTACLE_STOP_DISTANCE_CM = 20.0f;
const float OBSTACLE_RELEASE_DISTANCE_CM = 25.0f;

// O datasheet recomenda intervalo superior a 60 ms entre medições.
const unsigned long ULTRASONIC_INTERVAL_MS = 80;
const unsigned long ULTRASONIC_TIMEOUT_US = 30000;

// ============================================================
// Estado do robô
// ============================================================
int motorSpeed = 180;  // 0..255
char currentCommand = 'S';

const unsigned long COMMAND_TIMEOUT_MS = 900;
unsigned long lastMotionCommandMs = 0;

float lastDistanceCm = -1.0f;
bool obstacleDetected = false;
unsigned long lastUltrasonicReadMs = 0;

// ============================================================
// Página de controle
// ============================================================
const char INDEX_HTML[] = R"HTML(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  <title>Robô ESP32</title>
  <style>
    :root { font-family: Arial, sans-serif; }
    body {
      margin: 0;
      min-height: 100vh;
      display: flex;
      justify-content: center;
      background: #111;
      color: #fff;
    }
    .app {
      width: min(440px, 94vw);
      padding: 24px 0 40px;
      text-align: center;
    }
    h1 { margin: 0 0 8px; }
    .subtitle { color: #bbb; margin-bottom: 14px; }
    .sensor {
      margin: 12px auto 18px;
      padding: 12px 14px;
      border-radius: 14px;
      background: #1e1e1e;
      max-width: 330px;
    }
    .sensor.alert {
      background: #6d1515;
      color: #fff;
    }
    .pad {
      display: grid;
      grid-template-columns: repeat(3, 92px);
      grid-template-rows: repeat(3, 92px);
      gap: 10px;
      justify-content: center;
      margin: 22px 0;
    }
    button {
      border: 0;
      border-radius: 18px;
      font-size: 34px;
      font-weight: bold;
      background: #2b2b2b;
      color: white;
      box-shadow: 0 4px 10px rgba(0,0,0,.35);
      touch-action: none;
      user-select: none;
    }
    button:active { transform: scale(.96); background: #444; }
    button:disabled {
      opacity: .25;
      cursor: not-allowed;
      transform: none;
    }
    .stop { background: #8b1a1a; font-size: 20px; }
    .speed-box {
      background: #1e1e1e;
      padding: 18px;
      border-radius: 16px;
      margin-top: 18px;
    }
    input[type=range] { width: 100%; }
    .status {
      margin-top: 16px;
      color: #a9e6a9;
      min-height: 24px;
    }
  </style>
</head>
<body>
  <main class="app">
    <h1>Robô 2WD ESP32</h1>
    <div class="subtitle">Controle Wi-Fi</div>

    <div id="sensor" class="sensor">
      Ultrassom: <strong id="distance">--</strong> cm<br>
      <span id="sensorMessage">Área frontal livre</span>
    </div>

    <div class="pad">
      <div></div>
      <button data-dir="F" aria-label="Frente">▲</button>
      <div></div>

      <button data-dir="L" aria-label="Esquerda">◀</button>
      <button class="stop" data-dir="S" aria-label="Parar">STOP</button>
      <button data-dir="R" aria-label="Direita">▶</button>

      <div></div>
      <button data-dir="B" aria-label="Ré">▼</button>
      <div></div>
    </div>

    <section class="speed-box">
      <label for="speed">Velocidade: <strong id="speedValue">180</strong>/255</label>
      <input id="speed" type="range" min="0" max="255" value="180">
    </section>

    <div id="status" class="status">Pronto</div>
  </main>

<script>
  let repeatTimer = null;
  let activeDir = 'S';
  let obstacle = false;

  function request(url) {
    return fetch(url, { cache: 'no-store' });
  }

  function sendMove(dir) {
    activeDir = dir;

    request('/move?dir=' + encodeURIComponent(dir))
      .then(async response => {
        if (!response.ok) {
          const text = await response.text();
          document.getElementById('status').textContent = text;
          return;
        }

        document.getElementById('status').textContent =
          dir === 'S' ? 'Parado' : 'Comando: ' + dir;
      })
      .catch(() => {
        document.getElementById('status').textContent = 'Falha de comunicação';
      });
  }

  function startMove(dir) {
    if (obstacle && dir !== 'B' && dir !== 'S') {
      document.getElementById('status').textContent =
        'Obstáculo à frente: somente RÉ está liberada.';
      return;
    }

    if (repeatTimer) clearInterval(repeatTimer);
    sendMove(dir);

    if (dir !== 'S') {
      repeatTimer = setInterval(() => sendMove(dir), 250);
    }
  }

  function stopMove(sendStop = true) {
    if (repeatTimer) {
      clearInterval(repeatTimer);
      repeatTimer = null;
    }

    if (sendStop && activeDir !== 'S') {
      sendMove('S');
    } else {
      activeDir = 'S';
    }
  }

  document.querySelectorAll('button[data-dir]').forEach(btn => {
    const dir = btn.dataset.dir;

    btn.addEventListener('pointerdown', e => {
      e.preventDefault();
      if (dir === 'S') {
        stopMove(false);
        sendMove('S');
      } else {
        startMove(dir);
      }
    });

    btn.addEventListener('pointerup', e => {
      e.preventDefault();
      if (dir !== 'S') stopMove();
    });

    btn.addEventListener('pointercancel', () => stopMove());
    btn.addEventListener('pointerleave', e => {
      if (e.buttons) stopMove();
    });
  });

  window.addEventListener('blur', () => stopMove());
  document.addEventListener('visibilitychange', () => {
    if (document.hidden) stopMove();
  });

  const speed = document.getElementById('speed');
  const speedValue = document.getElementById('speedValue');

  speed.addEventListener('input', () => {
    speedValue.textContent = speed.value;
  });

  speed.addEventListener('change', () => {
    request('/speed?value=' + encodeURIComponent(speed.value)).catch(() => {});
  });

  function applyObstacleState(isObstacle) {
    obstacle = isObstacle;

    document.querySelectorAll('button[data-dir="F"], button[data-dir="L"], button[data-dir="R"]')
      .forEach(btn => btn.disabled = obstacle);

    const sensorBox = document.getElementById('sensor');
    const message = document.getElementById('sensorMessage');

    if (obstacle) {
      sensorBox.classList.add('alert');
      message.textContent = 'OBSTÁCULO: frente/lados bloqueados; somente RÉ.';

      if (activeDir !== 'B' && activeDir !== 'S') {
        stopMove(false);
        activeDir = 'S';
      }
    } else {
      sensorBox.classList.remove('alert');
      message.textContent = 'Área frontal livre';
    }
  }

  function refreshStatus() {
    request('/status')
      .then(response => response.json())
      .then(data => {
        document.getElementById('distance').textContent =
          data.distanceCm === null ? '--' : Number(data.distanceCm).toFixed(1);

        speed.value = data.speed;
        speedValue.textContent = data.speed;
        applyObstacleState(Boolean(data.obstacle));
      })
      .catch(() => {});
  }

  refreshStatus();
  setInterval(refreshStatus, 300);
</script>
</body>
</html>
)HTML";

// ============================================================
// Controle dos motores
// ============================================================

void setMotor(uint8_t in1, uint8_t in2, uint8_t pwmPin, int signedSpeed) {
  signedSpeed = constrain(signedSpeed, -255, 255);

  if (signedSpeed > 0) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    analogWrite(pwmPin, signedSpeed);
  } else if (signedSpeed < 0) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    analogWrite(pwmPin, -signedSpeed);
  } else {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    analogWrite(pwmPin, 0);
  }
}

void setLeftMotor(int speedValue) {
  setMotor(AIN1_PIN, AIN2_PIN, PWMA_PIN, speedValue);
}

void setRightMotor(int speedValue) {
  setMotor(BIN1_PIN, BIN2_PIN, PWMB_PIN, speedValue);
}

void stopMotors() {
  setLeftMotor(0);
  setRightMotor(0);
  currentCommand = 'S';
}

bool commandAllowedByObstacle(char command) {
  if (!obstacleDetected) {
    return true;
  }

  // Com obstáculo à frente, apenas ré e stop são permitidos.
  return command == 'B' || command == 'S';
}

bool executeCommand(char command) {
  command = toupper(command);

  if (!commandAllowedByObstacle(command)) {
    stopMotors();
    Serial.printf(
      "[ULTRASSOM] Movimento %c bloqueado. Obstáculo a %.1f cm. Use B (ré) ou S (stop).\n",
      command,
      lastDistanceCm
    );
    return false;
  }

  switch (command) {
    case 'F':
      setLeftMotor(motorSpeed);
      setRightMotor(motorSpeed);
      break;

    case 'B':
      setLeftMotor(-motorSpeed);
      setRightMotor(-motorSpeed);
      break;

    case 'L':
      // Giro sobre o próprio eixo para a esquerda.
      setLeftMotor(-motorSpeed);
      setRightMotor(motorSpeed);
      break;

    case 'R':
      // Giro sobre o próprio eixo para a direita.
      setLeftMotor(motorSpeed);
      setRightMotor(-motorSpeed);
      break;

    case 'S':
      stopMotors();
      Serial.println("[MOVIMENTO] STOP");
      return true;

    default:
      Serial.printf("[AVISO] Comando inválido: %c\n", command);
      return false;
  }

  currentCommand = command;
  lastMotionCommandMs = millis();

  Serial.printf(
    "[MOVIMENTO] comando=%c velocidade=%d\n",
    currentCommand,
    motorSpeed
  );

  return true;
}

// ============================================================
// Sensor ultrassônico HC-SR04
// ============================================================

float readUltrasonicDistanceCm() {
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);

  unsigned long durationUs = pulseIn(
    ULTRASONIC_ECHO_PIN,
    HIGH,
    ULTRASONIC_TIMEOUT_US
  );

  if (durationUs == 0) {
    return -1.0f;
  }

  return durationUs / 58.0f;
}

void updateUltrasonicSensor() {
  unsigned long now = millis();

  if (now - lastUltrasonicReadMs < ULTRASONIC_INTERVAL_MS) {
    return;
  }

  lastUltrasonicReadMs = now;

  float distanceCm = readUltrasonicDistanceCm();

  if (distanceCm < 0) {
    // Sem eco válido: mantém o estado anterior para evitar liberar o robô
    // por uma leitura isolada inválida.
    return;
  }

  lastDistanceCm = distanceCm;

  bool previousObstacleState = obstacleDetected;

  if (!obstacleDetected && distanceCm <= OBSTACLE_STOP_DISTANCE_CM) {
    obstacleDetected = true;
  } else if (obstacleDetected && distanceCm >= OBSTACLE_RELEASE_DISTANCE_CM) {
    obstacleDetected = false;
  }

  if (!previousObstacleState && obstacleDetected) {
    Serial.printf(
      "[ULTRASSOM] Obstáculo detectado a %.1f cm. Parando o robô.\n",
      distanceCm
    );

    // Ao detectar um obstáculo, para imediatamente qualquer movimento.
    // Depois da parada, apenas B (ré) e S (stop) ficam liberados.
    stopMotors();
  }

  if (previousObstacleState && !obstacleDetected) {
    Serial.printf(
      "[ULTRASSOM] Caminho liberado. Distância: %.1f cm.\n",
      distanceCm
    );
  }
}

// ============================================================
// Wi-Fi Scan - baseado no código fornecido
// ============================================================

void scanWiFiNetworks() {
  Serial.println();
  Serial.println("[WIFI] Scanning...");

  int n = WiFi.scanNetworks();

  Serial.println("[WIFI] Scan done!");

  if (n == 0) {
    Serial.println("[WIFI] No networks found.");
  } else {
    Serial.print("[WIFI] ");
    Serial.print(n);
    Serial.println(" networks found");

    for (int i = 0; i < n; ++i) {
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(" dBm)");
      Serial.println(
        (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " [OPEN]" : " [SECURED]"
      );
      delay(10);
    }
  }

  WiFi.scanDelete();
  Serial.println();
}

// ============================================================
// Servidor HTTP
// ============================================================

void handleRoot() {
  server.send(200, "text/html; charset=utf-8", INDEX_HTML);
}

void handleMove() {
  if (!server.hasArg("dir") || server.arg("dir").length() == 0) {
    server.send(400, "text/plain", "Parametro 'dir' ausente.");
    return;
  }

  char command = toupper(server.arg("dir").charAt(0));

  if (command != 'F' && command != 'B' &&
      command != 'L' && command != 'R' &&
      command != 'S') {
    server.send(400, "text/plain", "Comando invalido.");
    return;
  }

  bool accepted = executeCommand(command);

  if (!accepted && obstacleDetected) {
    server.send(
      423,
      "text/plain; charset=utf-8",
      "Obstáculo à frente: somente RÉ ou STOP estão liberados."
    );
    return;
  }

  server.send(200, "text/plain", "OK");
}

void handleSpeed() {
  if (!server.hasArg("value")) {
    server.send(400, "text/plain", "Parametro 'value' ausente.");
    return;
  }

  int newSpeed = constrain(server.arg("value").toInt(), 0, 255);
  motorSpeed = newSpeed;

  // Se o robô estiver em movimento, aplica a nova velocidade imediatamente.
  if (currentCommand != 'S') {
    executeCommand(currentCommand);
  }

  Serial.printf("[VELOCIDADE] %d/255\n", motorSpeed);
  server.send(200, "text/plain", String(motorSpeed));
}

void handleStatus() {
  String json = "{";
  json += "\"command\":\"";
  json += currentCommand;
  json += "\",\"speed\":";
  json += motorSpeed;
  json += ",\"clients\":";
  json += WiFi.softAPgetStationNum();
  json += ",\"obstacle\":";
  json += obstacleDetected ? "true" : "false";
  json += ",\"stopDistanceCm\":";
  json += String(OBSTACLE_STOP_DISTANCE_CM, 1);
  json += ",\"releaseDistanceCm\":";
  json += String(OBSTACLE_RELEASE_DISTANCE_CM, 1);
  json += ",\"distanceCm\":";

  if (lastDistanceCm < 0) {
    json += "null";
  } else {
    json += String(lastDistanceCm, 1);
  }

  json += "}";

  server.send(200, "application/json", json);
}

void startWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/move", HTTP_GET, handleMove);
  server.on("/speed", HTTP_GET, handleSpeed);
  server.on("/status", HTTP_GET, handleStatus);

  server.onNotFound([]() {
    server.send(404, "text/plain", "404 - Rota nao encontrada");
  });

  server.begin();
  Serial.println("[HTTP] Servidor iniciado.");
}

// ============================================================
// Serial Monitor - útil para Wokwi
// ============================================================

void printSerialHelp() {
  Serial.println("============================================");
  Serial.println("Robô 2WD ESP32 - comandos Serial:");
  Serial.println("  F = frente");
  Serial.println("  B = re");
  Serial.println("  L = esquerda");
  Serial.println("  R = direita");
  Serial.println("  S = parar");
  Serial.println("  + = aumentar velocidade em 15");
  Serial.println("  - = diminuir velocidade em 15");
  Serial.println();
  Serial.printf("Ultrassom: para em <= %.1f cm e libera em >= %.1f cm.\n",
                OBSTACLE_STOP_DISTANCE_CM,
                OBSTACLE_RELEASE_DISTANCE_CM);
  Serial.println("Com obstáculo, somente B (ré) e S (stop) são aceitos.");
  Serial.println("============================================");
}

void handleSerialCommands() {
  while (Serial.available() > 0) {
    char c = Serial.read();

    if (c == '\n' || c == '\r' || c == ' ') {
      continue;
    }

    c = toupper(c);

    if (c == '+') {
      motorSpeed = min(255, motorSpeed + 15);
      Serial.printf("[VELOCIDADE] %d/255\n", motorSpeed);

      if (currentCommand != 'S') {
        executeCommand(currentCommand);
      }
      continue;
    }

    if (c == '-') {
      motorSpeed = max(0, motorSpeed - 15);
      Serial.printf("[VELOCIDADE] %d/255\n", motorSpeed);

      if (currentCommand != 'S') {
        executeCommand(currentCommand);
      }
      continue;
    }

    executeCommand(c);
  }
}

// ============================================================
// Setup / Loop
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println();
  Serial.println("Initializing WiFi...");

  pinMode(AIN1_PIN, OUTPUT);
  pinMode(AIN2_PIN, OUTPUT);
  pinMode(PWMA_PIN, OUTPUT);

  pinMode(BIN1_PIN, OUTPUT);
  pinMode(BIN2_PIN, OUTPUT);
  pinMode(PWMB_PIN, OUTPUT);

  pinMode(STBY_PIN, OUTPUT);

  pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC_ECHO_PIN, INPUT);
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);

  stopMotors();

  // Habilita o TB6612FNG.
  digitalWrite(STBY_PIN, HIGH);

  // AP + STA:
  // - STA permite fazer o scan das redes ao redor.
  // - AP cria a rede própria do robô.
  WiFi.mode(WIFI_AP_STA);

  scanWiFiNetworks();

  WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);

  bool apStarted = WiFi.softAP(AP_SSID, AP_PASSWORD);

  if (apStarted) {
    Serial.println("[WIFI] Access Point criado!");
    Serial.print("[WIFI] SSID: ");
    Serial.println(AP_SSID);
    Serial.print("[WIFI] Senha: ");
    Serial.println(AP_PASSWORD);
    Serial.print("[WIFI] Abra no celular: http://");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("[ERRO] Falha ao iniciar o Access Point.");
  }

  startWebServer();
  printSerialHelp();

  Serial.println("Setup done!");
}

void loop() {
  server.handleClient();
  handleSerialCommands();
  updateUltrasonicSensor();

  // Fail-safe de comunicação:
  // Se o celular parar de enviar comandos enquanto o robô estiver andando,
  // o ESP32 para os motores automaticamente.
  if (currentCommand != 'S' &&
      millis() - lastMotionCommandMs > COMMAND_TIMEOUT_MS) {
    Serial.println("[SEGURANCA] Timeout de comando. Parando motores.");
    stopMotors();
  }

  delay(2);
}
