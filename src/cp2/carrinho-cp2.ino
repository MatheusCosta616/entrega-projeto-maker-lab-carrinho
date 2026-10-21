#include <WiFi.h>
#include <WebServer.h>
#include <cstring>
#include <guiespim18-project-1_inferencing.h>


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
#define DISTANCIA_EVASAO_CM 30.0f     // antecipa a evasao sem esperar a janela TinyML
#define DISTANCIA_MANUAL_MAXIMA_CM 30.0f
#define PWM_NORMAL 255
#define PWM_ATENCAO 140
#define PWM_GIRO 190
#define TEMPO_PARADA_ANTES_RECUO_MS 250
#define TEMPO_RECUO_MS 220
#define TEMPO_MEIA_VOLTA_MS 650       // calibrar no chao usado na demonstracao

const char* ssid = "ESP32_RC";
const char* password = "esp32pass";

WebServer server(80);

// O buffer guarda uma janela deslizante do mesmo tamanho usado no treinamento.
float amostrasTinyML[EI_CLASSIFIER_RAW_SAMPLE_COUNT];
size_t numeroAmostras = 0;
unsigned long ultimaLeituraMs = 0;
unsigned long ultimoDisparoSensorMs = 0;

bool andandoFrente = false;
bool giroPerigoAtivo = false;
bool bloqueioPerigo = false;
bool perigoJaTratado = false;
bool modoAutonomo = false;
bool recuoAntesDoGiro = false;
bool paradaAntesDoRecuo = false;
uint8_t inferenciasSegurasSeguidas = 0;
unsigned long fimGiroMs = 0;

char classeTinyML[24] = "aguardando";
char decisaoTinyML[64] = "aguardando leituras";
float confiancaTinyML = 0.0f;
float ultimaDistancia = -1.0f;
int velocidadeAtual = PWM_ATENCAO;  // começa devagar até a primeira inferência

// =========================
// SENSOR HC-SR04
// =========================
float medirDistancia() {
  // Respeita o intervalo minimo entre pulsos do HC-SR04 para evitar eco antigo.
  while (ultimoDisparoSensorMs != 0 &&
         millis() - ultimoDisparoSensorMs < 60UL) {
    delay(1);
  }
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  ultimoDisparoSensorMs = millis();

  const unsigned long duracao = pulseIn(ECHO_PIN, HIGH, 25000UL);
  if (duracao == 0) return -1.0f;  // tentar novamente no proximo ciclo
  return duracao * 0.0343f / 2.0f;
}

void definirClasse(const char* classe) {
  strncpy(classeTinyML, classe, sizeof(classeTinyML) - 1);
  classeTinyML[sizeof(classeTinyML) - 1] = '\0';
}

void definirDecisao(const char* decisao) {
  strncpy(decisaoTinyML, decisao, sizeof(decisaoTinyML) - 1);
  decisaoTinyML[sizeof(decisaoTinyML) - 1] = '\0';
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
  recuoAntesDoGiro = false;
  paradaAntesDoRecuo = false;
  definirDecisao(motivo);
  Serial.println(motivo);
}

void aplicarPwmFrente(int pwm) {
  // Os dois canais de habilitacao devem receber o mesmo PWM para tracao reta.
  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(IN3_PIN, HIGH);
  digitalWrite(IN4_PIN, LOW);
  analogWrite(ENA_PIN, pwm);
  analogWrite(ENB_PIN, pwm);
  velocidadeAtual = pwm;
  andandoFrente = true;
  definirDecisao(pwm == PWM_NORMAL ? "Avancando" : "Avancando devagar");
}

void frente() {
  // No manual, o TinyML nao participa: o sensor aplica a protecao do CP1.
  const float distancia = medirDistancia();
  ultimaDistancia = distancia;
  if (distancia < 0.0f) {
    pararMotores("Sem eco do HC-SR04; frente bloqueada por seguranca");
    return;
  }
  if (distancia <= DISTANCIA_MANUAL_MAXIMA_CM) {
    pararMotores("Obstaculo a 30 cm ou menos; frente bloqueada");
    return;
  }
  aplicarPwmFrente(PWM_NORMAL);
  Serial.printf("FORWARD manual | PWM %d | %.1f cm\n", PWM_NORMAL, distancia);
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
  definirDecisao("Re manual");
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
  definirDecisao("Curva manual a esquerda");
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
  definirDecisao("Curva manual a direita");
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
  recuoAntesDoGiro = true;
  paradaAntesDoRecuo = true;
  fimGiroMs = millis() + TEMPO_PARADA_ANTES_RECUO_MS;
  analogWrite(ENA_PIN, 0);
  analogWrite(ENB_PIN, 0);
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(IN3_PIN, LOW);
  digitalWrite(IN4_PIN, LOW);
  definirDecisao("Perigo: parando antes de recuar");
  Serial.println("Perigo: pausa de seguranca antes do recuo e giro");
}

void iniciarRecuo() {
  paradaAntesDoRecuo = false;
  recuoAntesDoGiro = true;
  fimGiroMs = millis() + TEMPO_RECUO_MS;
  definirDecisao("Perigo: recuando antes de virar");
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, HIGH);
  digitalWrite(IN3_PIN, LOW);
  digitalWrite(IN4_PIN, HIGH);
  analogWrite(ENA_PIN, PWM_GIRO);
  analogWrite(ENB_PIN, PWM_GIRO);
  Serial.println("Evasao: recuando");
}

void iniciarRotacao() {
  recuoAntesDoGiro = false;
  paradaAntesDoRecuo = false;
  fimGiroMs = millis() + TEMPO_MEIA_VOLTA_MS;
  definirDecisao("Perigo: girando 180");
  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(IN3_PIN, LOW);
  digitalWrite(IN4_PIN, HIGH);
  analogWrite(ENA_PIN, PWM_GIRO);
  analogWrite(ENB_PIN, PWM_GIRO);

  Serial.println("Evasao: giro de meia volta");
}

void atualizarGiro() {
  if (giroPerigoAtivo && (long)(millis() - fimGiroMs) >= 0) {
    if (paradaAntesDoRecuo) {
      iniciarRecuo();
      return;
    }
    if (recuoAntesDoGiro) {
      iniciarRotacao();
      return;
    }

    pararMotores("Giro concluido; aguardando leituras novas");
    // Descarta a janela que ainda poderia conter distancias da parede antiga.
    numeroAmostras = 0;
    confiancaTinyML = 0.0f;
    inferenciasSegurasSeguidas = 0;
    definirClasse("aguardando");
    perigoJaTratado = false;
    ultimaLeituraMs = millis();
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
  // Nao permita que uma classificacao durante a manobra interrompa pausa/recuo/giro.
  if (giroPerigoAtivo) return;

  const bool confiavel = confiancaTinyML >= CONFIANCA_MINIMA;

  if (!confiavel) {
    inferenciasSegurasSeguidas = 0;
    velocidadeAtual = 0;
    definirDecisao("Baixa confianca: aguardando");
    if (modoAutonomo && !giroPerigoAtivo) {
      pararMotores("AUTO: baixa confianca; parado");
    } else if (andandoFrente && !giroPerigoAtivo) {
      aplicarPwmFrente(PWM_ATENCAO);
    }
    return;
  }

  if (confiavel && strcmp(classeTinyML, "perigo") == 0) {
    inferenciasSegurasSeguidas = 0;
    bloqueioPerigo = true;
    velocidadeAtual = 0;
    if (!perigoJaTratado) {
      if (modoAutonomo || andandoFrente) iniciarMeiaVolta();
      else {
        perigoJaTratado = true;
        pararMotores("TinyML: perigo detectado; frente bloqueada");
      }
    } else if (modoAutonomo && !giroPerigoAtivo && andandoFrente) {
      pararMotores("AUTO: perigo detectado; parado");
    }
    return;
  }

  if (confiavel && strcmp(classeTinyML, "atencao") == 0) {
    inferenciasSegurasSeguidas = 0;
    velocidadeAtual = 0;
    if (modoAutonomo || andandoFrente) {
      pararMotores("TinyML: atencao; carrinho parado");
    } else {
      definirDecisao("TinyML: atencao; aguardando");
    }
    return;
  }

  if (confiavel && strcmp(classeTinyML, "livre") == 0) {
    if (inferenciasSegurasSeguidas < 3) inferenciasSegurasSeguidas++;
    // Exige tres inferencias seguras para confirmar que a frente esta livre.
    if (modoAutonomo && inferenciasSegurasSeguidas < 3) {
      velocidadeAtual = 0;
      if (!andandoFrente && !giroPerigoAtivo) {
        definirDecisao("Validando 3 leituras seguras");
      }
      return;
    }
    bloqueioPerigo = false;
    perigoJaTratado = false;
    velocidadeAtual = PWM_NORMAL;
  }

  if (modoAutonomo && !giroPerigoAtivo) {
    // No modo autonomo, uma classe segura valida retoma a marcha sozinha.
    aplicarPwmFrente(velocidadeAtual);
  } else if (andandoFrente && !giroPerigoAtivo) {
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
    inferenciasSegurasSeguidas = 0;
    velocidadeAtual = PWM_ATENCAO;
    if (giroPerigoAtivo) {
      // Durante recuo/giro o HC-SR04 pode apontar para o lado e perder o eco.
      // Deixe a manobra temporizada terminar; nao classifique -1 como distancia.
      definirDecisao(recuoAntesDoGiro
                       ? "Sem eco durante recuo; manobra continua"
                       : "Sem eco durante giro; manobra continua");
      Serial.println("HC-SR04 sem eco durante evasao; mantendo a manobra");
      return;
    }
    definirDecisao("Sem eco: aguardando leitura valida");
    Serial.println("HC-SR04 sem eco; leitura ignorada");
    if (andandoFrente || modoAutonomo) {
      pararMotores("Parada de seguranca: sem eco do HC-SR04");
    }
    return;
  }

  // No modo manual, o sensor apenas mantem a protecao de proximidade do CP1.
  // O classificador e as classes TinyML nao podem comandar os motores aqui.
  if (!modoAutonomo) {
    numeroAmostras = 0;
    confiancaTinyML = 0.0f;
    definirClasse("manual");
    definirDecisao("Modo manual: aguardando comando do celular");
    if (andandoFrente && distancia <= DISTANCIA_MANUAL_MAXIMA_CM) {
      pararMotores("CP1 manual: obstaculo a 30 cm ou menos; parado");
    }
    return;
  }

  // Corte de emergencia extrema; nas distancias normais, a acao vem do modelo.
  if (distancia <= DISTANCIA_EMERGENCIA_CM) {
    numeroAmostras = 0;
    bloqueioPerigo = true;
    inferenciasSegurasSeguidas = 0;
    perigoJaTratado = false;  // ao sair da zona critica, o modelo ainda pode acionar evasao
    definirDecisao("Emergencia: pausa, recuo e giro");
    iniciarMeiaVolta();
    return;
  }

  // Protecao rapida: nao espera a janela de 21 leituras quando a parede esta perto.
  if (modoAutonomo && distancia <= DISTANCIA_EVASAO_CM) {
    numeroAmostras = 0;
    inferenciasSegurasSeguidas = 0;
    bloqueioPerigo = true;
    if (!giroPerigoAtivo && !perigoJaTratado) {
      definirDecisao("Distancia curta: iniciando evasao");
      iniciarMeiaVolta();
    }
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
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}body{margin:0;background:#111;color:white;font-family:Arial,sans-serif;text-align:center;user-select:none;touch-action:manipulation}h1{margin-top:25px;font-size:28px}.controls{width:300px;margin:20px auto;display:grid;grid-template-columns:90px 90px 90px;grid-template-rows:90px 90px 90px;gap:15px;justify-content:center}button{width:90px;height:90px;font-size:42px;border:0;border-radius:20px;background:#333;color:white;touch-action:manipulation}.forward{grid-column:2;grid-row:1}.left{grid-column:1;grid-row:2}.stop{grid-column:2;grid-row:2;background:#b00000}.right{grid-column:3;grid-row:2}.backward{grid-column:2;grid-row:3}.auto{width:min(300px,90vw);height:58px;font-size:20px;background:#176b3a;border-radius:12px}.autooff{background:#555}#status{margin:16px auto;padding:12px;max-width:360px;color:#ddd;background:#222;border-radius:10px;line-height:1.5}
</style>
</head>
<body>
<h1>ESP32 RC CAR</h1>
<button id="autoButton" class="auto" onclick="alternarAuto()">LIGAR MODO AUTO</button>
<div class="controls">
<button class="forward" onclick="sendCommand('forward')">&#8593;</button>
<button class="left" onclick="sendCommand('left')">&#8592;</button>
<button class="stop" onclick="sendCommand('stop')">&#9632;</button>
<button class="right" onclick="sendCommand('right')">&#8594;</button>
<button class="backward" onclick="sendCommand('backward')">&#8595;</button>
</div>
<div id="status">TinyML inicializando...</div>
<script>
function atualizar(){fetch('/status').then(r=>r.json()).then(s=>{const d=Number(s.distancia)<0?'sem eco':s.distancia+' cm';document.getElementById('status').innerHTML='Modo: <b>'+s.modo+'</b><br>Decisao: '+s.decisao+'<br>Classe: '+s.classe+' ('+s.confianca+'%)<br>Distancia: '+d+' | PWM: '+s.pwm;const b=document.getElementById('autoButton');b.textContent=s.auto?'DESLIGAR MODO AUTO':'LIGAR MODO AUTO';b.className=s.auto?'auto autooff':'auto';}).catch(()=>{});}
setInterval(atualizar,500);
function sendCommand(cmd){fetch('/command?cmd='+cmd).then(()=>atualizar()).catch(()=>{});}
function alternarAuto(){fetch('/command?cmd='+((document.getElementById('autoButton').textContent==='LIGAR MODO AUTO')?'auto_on':'auto_off')).then(()=>atualizar()).catch(()=>{});}
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
  if (cmd == "auto_on") {
    modoAutonomo = true;
    perigoJaTratado = false;
    bloqueioPerigo = false;
    numeroAmostras = 0;  // comeca com uma janela fresca ao ativar o automatico
    inferenciasSegurasSeguidas = 0;
    confiancaTinyML = 0.0f;
    definirClasse("aguardando");
    pararMotores("Modo AUTO ligado; aguardando novas leituras");
    server.send(200, "text/plain", "Modo AUTO ligado");
    return;
  }
  if (cmd == "auto_off") {
    modoAutonomo = false;
    inferenciasSegurasSeguidas = 0;
    pararMotores("Modo AUTO desligado");
    server.send(200, "text/plain", "Modo AUTO desligado");
    return;
  }

  // Comando do celular assume controle manual e cancela evasao automatica em curso.
  if (modoAutonomo && giroPerigoAtivo) {
    pararMotores("Modo manual: manobra automatica cancelada");
  }
  modoAutonomo = false;
  if (cmd == "forward") frente();
  else if (cmd == "backward") re();
  else if (cmd == "left") esquerda();
  else if (cmd == "right") direita();
  else if (cmd == "stop") pararMotores("STOP manual; AUTO desligado");
  else {
    server.send(400, "text/plain", "Comando desconhecido");
    return;
  }

  server.send(200, "text/plain", "OK | TinyML: " + String(classeTinyML));
}

void handleStatus() {
  char texto[256];
  snprintf(texto, sizeof(texto),
           "{\"auto\":%s,\"modo\":\"%s\",\"decisao\":\"%s\",\"classe\":\"%s\",\"confianca\":%.0f,\"distancia\":%.1f,\"pwm\":%d}",
           modoAutonomo ? "true" : "false",
           modoAutonomo ? "AUTONOMO" : "MANUAL",
           decisaoTinyML, classeTinyML, confiancaTinyML * 100.0f,
           ultimaDistancia, velocidadeAtual);
  server.send(200, "application/json", texto);
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
