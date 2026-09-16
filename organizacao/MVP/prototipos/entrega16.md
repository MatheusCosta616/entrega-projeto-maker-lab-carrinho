# Programação e Testes Iniciais - Carrinho Robô

## Objetivo

Implementar e validar o sistema de controle do carrinho robô utilizando Arduino ou ESP32, integrando motores, ponte H, sensores e comunicação sem fio.

---

# Arquitetura do Sistema

```text
             +----------------+
             |    Sensor      |
             +--------+-------+
                      |
                      v

             +----------------+
             | Arduino / ESP32|
             +--------+-------+
                      ^
                      |
             +--------+-------+
             | Controle Sem   |
             | Fio            |
             | WiFi / BLE     |
             +--------+-------+
                      |
                      v

             +----------------+
             |    Ponte H     |
             +-----+-----+----+
                   |     |
                   v     v

              +------+ +------+
              |Motor | |Motor |
              | Esq. | | Dir. |
              +------+ +------+
```

# Programação Modular

```cpp
void frente(){}
void tras(){}
void esquerda(){}
void direita(){}
void parar(){}
```

# Testes Incrementais

```text
Motor Esquerdo
      ↓
    TESTAR
      ↓
Motor Direito
      ↓
    TESTAR
      ↓
 Frente()
      ↓
    TESTAR
      ↓
 Tras()
      ↓
    TESTAR
      ↓
Esquerda()
      ↓
    TESTAR
      ↓
Direita()
```

# Controle via WiFi (ESP32)

## Ligações

- IN1 → GPIO 27
- IN2 → GPIO 26
- ENA → GPIO 14
- IN3 → GPIO 33
- IN4 → GPIO 25
- ENB → GPIO 32

## Bibliotecas

```cpp
#include <WiFi.h>
#include <WebServer.h>
```

## Configuração dos Motores

```cpp
int motor1Pin1 = 27;
int motor1Pin2 = 26;
int enable1Pin = 14;

int motor2Pin1 = 33;
int motor2Pin2 = 25;
int enable2Pin = 32;
```

## Movimento Frente

```cpp
void handleForward() {
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, HIGH);
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, HIGH);
}
```

## Movimento Ré

```cpp
void handleReverse() {
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor2Pin1, HIGH);
  digitalWrite(motor2Pin2, LOW);
}
```

## Parar

```cpp
void handleStop() {
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, LOW);
}
```

## Controle PWM

```cpp
const int freq = 30000;
const int resolution = 8;

int dutyCycle = 0;
```

```cpp
dutyCycle = map(value, 25, 100, 200, 255);
ledcWrite(enable1Pin, dutyCycle);
ledcWrite(enable2Pin, dutyCycle);
```

# Controle BLE (ESP32)

## Bibliotecas

```cpp
#include <DabbleESP32.h>
```

## Pinos

```cpp
#define IN1 16
#define IN2 17
#define IN3 18
#define IN4 19
#define ENA 5
#define ENB 6
```

## Setup

```cpp
void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  Dabble.begin("My Bluetooth Car");
}
```

## Funções BLE

```cpp
void moveForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}
```

```cpp
void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}
```

# Controle Bluetooth HC-05 (Arduino)

## Bibliotecas

```cpp
#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE
#include <Dabble.h>
```

## Setup

```cpp
void setup() {
  Serial.begin(250000);
  Dabble.begin(9600);
}
```

## Movimento Frente

```cpp
void moveParaFrente() {
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
}
```

## Movimento Ré

```cpp
void moveParaTras() {
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
}
```

## Parar

```cpp
void parar() {
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}
```

# Estrutura do Projeto

```text
Projeto-Carrinho-Robo
│
├── hardware
│   ├── arquitetura
│   └── componentes
├── cad
│   ├── STL
│   └── fonte
├── src
│   └── codigo.ino
└── README.md
```

# Entregáveis

- Testes dos códigos
- Integração dos sensores
- Registro em foto ou vídeo
- Código .ino no GitHub
- Arquitetura do hardware
- Arquivos CAD/STL
