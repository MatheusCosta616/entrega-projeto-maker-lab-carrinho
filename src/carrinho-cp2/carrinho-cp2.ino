#include <WiFi.h>
#include <WebServer.h>
#include <cstring>

// Use o nome gerado pelo projeto Edge Impulse atual ou o nome legado.
#if __has_include(<guiespim18_project_1_inferencing.h>)
  #include <guiespim18_project_1_inferencing.h>
#elif __has_include(<CP2_TinyML_ESP32_inferencing.h>)
  #include <CP2_TinyML_ESP32_inferencing.h>
#else
  #error "Instale a biblioteca C++ do Edge Impulse (Deployment v2) na Arduino IDE."
#endif

// =========================
// PINOS: iguais ao firmware CP1
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
// CONFIGURACAO TinyML / motores
// =========================
#define CONFIANCA_MINIMA 0.60f
#define DISTANCIA_EMERGENCIA_CM 8.0f  // parada de seguranca independente do modelo
#define PWM_NORMAL 255
#define PWM_ATENCAO 140
#define PWM_GIRO 190
#define TEMPO_MEIA_VOLTA_MS 650       // calibrar no chao usado na demonstracao

const char* ssid = "ESP32_RC";
const char* password = "esp32pass";

WebServer server(80);

// O buffer guarda uma janela deslizante do mesmo tamanho usado no treinamento.
float amostrasTinyML[EI_CLASSIFIER_RAW_SAMPLE_COUNT];
size_t numeroAmostras = 0;
unsigned long ultimaLeituraMs = 0;

bool andandoFrente = false;
bool giroPerigoAtivo = false;
bool bloqueioPerigo = false;
bool perigoJaTratado = false;
unsigned long fimGiroMs = 0;

char classeTinyML[24] = "aguardando";
float confiancaTinyML = 0.0f;
float ultimaDistancia = -1.0f;
int velocidadeAtual = PWM_ATENCAO;  // começa devagar até a primeira inferência

// =========================
// SENSOR HC-SR04
// =========================
float medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duracao = pulseIn(ECHO_PIN, HIGH, 30000UL);
  if (duracao == 0) return -1.0f;
  return duracao * 0.0343f / 2.0f;
}

void definirClasse(const char* classe) {
  strncpy(classeTinyML, classe, sizeof(classeTinyML) - 1);
  classeTinyML[sizeof(classeTinyML) - 1] = '\0';
}

// =========================
// MOTORES
// =========================
void pararMotores(const char* motivo = "STOP") {
  analogWrite(ENA_PIN, 0);
  analogWrite(ENB_PIN, 0);
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(IN3_PIN, LOW);
  digitalWrite(IN4_PIN, LOW);
  andandoFrente = false;
  giroPerigoAtivo = false;
  Serial.println(motivo);
}

void aplicarPwmFrente(int pwm) {
  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(IN3_PIN, HIGH);
  digitalWrite(IN4_PIN, LOW);
  analogWrite(ENA_PIN, pwm);
  analogWrite(ENB_PIN, pwm);
  velocidadeAtual = pwm;
  andandoFrente = true;
}

void frente() {
  if (giroPerigoAtivo) return;
  if (ultimaDistancia < 0.0f) {
    pararMotores("Sem leitura valida do sensor: frente bloqueada");
    return;
  }
  if (bloqueioPerigo ||
      (strcmp(classeTinyML, "perigo") == 0 && confiancaTinyML >= CONFIANCA_MINIMA)) {
    pararMotores("Frente bloqueada pelo TinyML: perigo");
    return;
  }

  // Mantém a velocidade reduzida até o modelo reconhecer uma classe segura.
  const int pwm = (strcmp(classeTinyML, "livre") == 0 &&
                   confiancaTinyML >= CONFIANCA_MINIMA)
                    ? PWM_NORMAL : PWM_ATENCAO;
  aplicarPwmFrente(pwm);
  Serial.printf("FORWARD | PWM %d\n", pwm);
}

void re() {
  giroPerigoAtivo = false;
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, HIGH);
  digitalWrite(IN3_PIN, LOW);
  digitalWrite(IN4_PIN, HIGH);
  analogWrite(ENA_PIN, PWM_NORMAL);
  analogWrite(ENB_PIN, PWM_NORMAL);
  andandoFrente = false;
  Serial.println("BACKWARD");
}

void esquerda() {
  giroPerigoAtivo = false;
  analogWrite(ENA_PIN, 0);
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(IN3_PIN, HIGH);
  digitalWrite(IN4_PIN, LOW);
  analogWrite(ENB_PIN, PWM_NORMAL);
  andandoFrente = false;
  Serial.println("LEFT");
}

void direita() {
  giroPerigoAtivo = false;
  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);
  analogWrite(ENA_PIN, PWM_NORMAL);
  analogWrite(ENB_PIN, 0);
  digitalWrite(IN3_PIN, LOW);
  digitalWrite(IN4_PIN, LOW);
  andandoFrente = false;
  Serial.println("RIGHT");
}

// Giro aproximado de 180 graus: roda A para frente e roda B para tras.
// Sem encoder/giroscopio o angulo depende do piso, da bateria e dos motores.
void iniciarMeiaVolta() {
  if (giroPerigoAtivo) return;

  bloqueioPerigo = true;
  perigoJaTratado = true;
  andandoFrente = false;
  giroPerigoAtivo = true;
  fimGiroMs = millis() + TEMPO_MEIA_VOLTA_MS;

  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(IN3_PIN, LOW);
  digitalWrite(IN4_PIN, HIGH);
  analogWrite(ENA_PIN, PWM_GIRO);
  analogWrite(ENB_PIN, PWM_GIRO);

  Serial.println("TinyML: PERIGO - giro de meia volta");
}

void atualizarGiro() {
  if (giroPerigoAtivo && (long)(millis() - fimGiroMs) >= 0) {
    pararMotores("Giro concluido; aguardando classe segura para liberar frente");
    // Mantem bloqueioPerigo: o usuario pode usar re/curvas; frente so libera
    // quando uma inferencia confiavel identificar atencao ou livre.
  }
}

// =========================
// EDGE IMPULSE
// =========================
int obterDadosTinyML(size_t offset, size_t length, float *saida) {
  if (offset + length > EI_CLASSIFIER_RAW_SAMPLE_COUNT) return -1;
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
    definirClasse("erro");
    confiancaTinyML = 0.0f;
    Serial.printf("Erro TinyML: %d\n", erro);
    return false;
  }

  confiancaTinyML = 0.0f;
  definirClasse("indefinido");
  for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    const float probabilidade = resultado.classification[i].value;
    Serial.printf("%s: %.3f\n",
                  resultado.classification[i].label, probabilidade);
    if (probabilidade > confiancaTinyML) {
      confiancaTinyML = probabilidade;
      definirClasse(resultado.classification[i].label);
    }
  }

  Serial.printf("Classe TinyML: %s (%.1f%%) | distancia %.1f cm\n",
                classeTinyML, confiancaTinyML * 100.0f, ultimaDistancia);
  return true;
}

void aplicarDecisaoTinyML() {
  const bool confiavel = confiancaTinyML >= CONFIANCA_MINIMA;

  if (confiavel && strcmp(classeTinyML, "perigo") == 0) {
    bloqueioPerigo = true;
    velocidadeAtual = 0;
    if (!perigoJaTratado) {
      if (andandoFrente) iniciarMeiaVolta();
      else {
        perigoJaTratado = true;
        pararMotores("TinyML: perigo detectado; frente bloqueada");
      }
    }
    return;
  }

  if (confiavel && (strcmp(classeTinyML, "livre") == 0 ||
                    strcmp(classeTinyML, "atencao") == 0)) {
    // So uma classe segura com confianca suficiente libera novo comando de frente.
    bloqueioPerigo = false;
    perigoJaTratado = false;
    velocidadeAtual = strcmp(classeTinyML, "livre") == 0
                        ? PWM_NORMAL : PWM_ATENCAO;
  } else {
    // Baixa confianca: comportamento conservador, velocidade reduzida.
    velocidadeAtual = PWM_ATENCAO;
  }

  if (andandoFrente && !giroPerigoAtivo) {
    aplicarPwmFrente(velocidadeAtual);
  }
}

void atualizarTinyML() {
  if (millis() - ultimaLeituraMs < EI_CLASSIFIER_INTERVAL_MS) return;
  ultimaLeituraMs = millis();

  const float distancia = medirDistancia();
  ultimaDistancia = distancia;

  if (distancia < 0.0f) {
    numeroAmostras = 0;
    confiancaTinyML = 0.0f;
    definirClasse("sem_leitura");
    velocidadeAtual = PWM_ATENCAO;
    if (andandoFrente || giroPerigoAtivo) {
      pararMotores("Parada de seguranca: sem eco do HC-SR04");
    }
    return;
  }

  // Corte de emergencia extrema; nas distancias normais, a acao vem do modelo.
  if (distancia <= DISTANCIA_EMERGENCIA_CM) {
    numeroAmostras = 0;
    bloqueioPerigo = true;
    perigoJaTratado = true;
    pararMotores("Parada de emergencia: obstaculo a menos de 8 cm");
    return;
  }

  // Mantem os valores em ordem cronologica e classifica a janela deslizante.
  if (numeroAmostras < EI_CLASSIFIER_RAW_SAMPLE_COUNT) {
    amostrasTinyML[numeroAmostras++] = distancia;
  } else {
    memmove(amostrasTinyML, amostrasTinyML + 1,
            (EI_CLASSIFIER_RAW_SAMPLE_COUNT - 1) * sizeof(float));
    amostrasTinyML[EI_CLASSIFIER_RAW_SAMPLE_COUNT - 1] = distancia;
  }

  if (numeroAmostras == EI_CLASSIFIER_RAW_SAMPLE_COUNT && classificarTinyML()) {
    aplicarDecisaoTinyML();
  }
}

// =========================
// INTERFACE WEB (mesmos comandos do CP1 + status TinyML)
// =========================
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>ESP32 RC CAR - CP2 TinyML</title>
<style>
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}body{margin:0;background:#111;color:white;font-family:Arial,sans-serif;text-align:center;user-select:none;touch-action:manipulation}h1{margin-top:25px;font-size:28px}.controls{width:300px;margin:30px auto;display:grid;grid-template-columns:90px 90px 90px;grid-template-rows:90px 90px 90px;gap:15px;justify-content:center}button{width:90px;height:90px;font-size:42px;border:0;border-radius:20px;background:#333;color:white;touch-action:manipulation}.forward{grid-column:2;grid-row:1}.left{grid-column:1;grid-row:2}.stop{grid-column:2;grid-row:2;background:#b00000}.right{grid-column:3;grid-row:2}.backward{grid-column:2;grid-row:3}#status{margin:16px auto;padding:12px;max-width:360px;color:#ddd;background:#222;border-radius:10px}
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
<div id="status">TinyML inicializando...</div>
<script>
function atualizar(){fetch('/status').then(r=>r.text()).then(t=>document.getElementById('status').textContent=t).catch(()=>{});}
setInterval(atualizar,500);
function sendCommand(cmd){fetch('/command?cmd='+cmd).then(r=>r.text()).then(t=>{document.getElementById('status').textContent=t;atualizar();}).catch(()=>{});}
</script>
</body>
</html>
)rawliteral";

void handleCommand() {
  if (!server.hasArg("cmd")) {
    server.send(400, "text/plain", "Comando ausente");
    return;
  }

  const String cmd = server.arg("cmd");
  if (cmd == "forward") frente();
  else if (cmd == "backward") re();
  else if (cmd == "left") esquerda();
  else if (cmd == "right") direita();
  else if (cmd == "stop") pararMotores("STOP manual");
  else {
    server.send(400, "text/plain", "Comando desconhecido");
    return;
  }

  server.send(200, "text/plain", "OK | TinyML: " + String(classeTinyML));
}

void handleStatus() {
  char texto[128];
  snprintf(texto, sizeof(texto),
           "TinyML: %s (%.0f%%) | Distancia: %.1f cm | PWM: %d%s",
           classeTinyML, confiancaTinyML * 100.0f, ultimaDistancia,
           velocidadeAtual, giroPerigoAtivo ? " | GIRANDO 180" : "");
  server.send(200, "text/plain", texto);
}

// =========================
// SETUP / LOOP
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
  pararMotores("Motores parados ao iniciar");

  WiFi.softAP(ssid, password);
  Serial.println();
  Serial.println("CP2 TinyML - ESP32 RC CAR");
  Serial.printf("Wi-Fi: %s\nIP: %s\n", ssid, WiFi.softAPIP().toString().c_str());
  Serial.printf("Janela TinyML: %u leituras | intervalo: %u ms\n",
                (unsigned)EI_CLASSIFIER_RAW_SAMPLE_COUNT,
                (unsigned)EI_CLASSIFIER_INTERVAL_MS);
  Serial.println("Conecte ao Wi-Fi ESP32_RC e abra 192.168.4.1");

  server.on("/", []() {
    server.send_P(200, "text/html; charset=UTF-8", webpage);
  });
  server.on("/command", handleCommand);
  server.on("/status", handleStatus);
  server.begin();
}

void loop() {
  server.handleClient();
  atualizarGiro();
  atualizarTinyML();
  delay(1);
}
