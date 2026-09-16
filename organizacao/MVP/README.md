# Robô 2WD com ESP32, Wi-Fi e sensor ultrassônico

Projeto de um robô móvel **2WD** controlado por **ESP32**, com driver **TB6612FNG**, dois motores TT e um sensor ultrassônico **HC-SR04** montado na frente.

O robô pode ser comandado pelo celular através de uma página web hospedada no próprio ESP32 e possui uma proteção frontal automática contra colisões.

## Funcionalidades

- controle pelo celular via Wi-Fi;
- ESP32 cria a própria rede, sem precisar de roteador;
- frente, ré, esquerda, direita e stop;
- controle de velocidade dos motores;
- sensor ultrassônico frontal HC-SR04;
- parada automática ao detectar obstáculo próximo;
- com obstáculo à frente, **somente RÉ e STOP ficam permitidos**;
- fail-safe de comunicação;
- simulação no Wokwi;
- TB6612FNG representado por custom chip no Wokwi;
- dois motores visuais na simulação.

## Regra do sensor frontal

O firmware usa duas distâncias para evitar que o sistema fique alternando rapidamente entre bloqueado e liberado quando um objeto está exatamente no limite:

- **20 cm ou menos:** detecta obstáculo, para e bloqueia `F`, `L` e `R`;
- **enquanto bloqueado:** apenas `B` (ré) e `S` (stop) são aceitos;
- **25 cm ou mais:** considera o caminho liberado novamente.

Esses valores ficam no início do `sketch.ino`:

```cpp
const float OBSTACLE_STOP_DISTANCE_CM = 20.0f;
const float OBSTACLE_RELEASE_DISTANCE_CM = 25.0f;
```

## Arquitetura

```text
                         HC-SR04
                            |
                            | TRIG / ECHO
                            v
Celular ---- Wi-Fi -----> ESP32
                            |
                            | GPIO / PWM
                            v
                        TB6612FNG
                         /      \
                        /        \
               Motor esquerdo   Motor direito
                    + roda          + roda
```

## Rede Wi-Fi do robô

No hardware real, o ESP32 cria a rede:

- **SSID:** `Robo-ESP32`
- **Senha:** `fiap2026`
- **Painel:** `http://192.168.4.1`

O celular se conecta diretamente ao robô. Não é necessário internet.

## Comandos

| Comando | Movimento | Permitido com obstáculo frontal? |
|---|---|---|
| `F` | Frente | Não |
| `B` | Ré | **Sim** |
| `L` | Girar para esquerda | Não |
| `R` | Girar para direita | Não |
| `S` | Parar | **Sim** |
| `+` | Aumentar velocidade no Serial | Sim |
| `-` | Diminuir velocidade no Serial | Sim |

## Pinos usados

### TB6612FNG

| ESP32 | TB6612FNG | Função |
|---|---|---|
| GPIO 25 | AIN1 | Direção motor esquerdo |
| GPIO 26 | AIN2 | Direção motor esquerdo |
| GPIO 27 | PWMA | Velocidade motor esquerdo |
| GPIO 32 | BIN1 | Direção motor direito |
| GPIO 33 | BIN2 | Direção motor direito |
| GPIO 14 | PWMB | Velocidade motor direito |
| GPIO 13 | STBY | Habilita o driver |
| 3V3 | VCC | Alimentação lógica |
| GND | GND | Terra comum |

### HC-SR04

| HC-SR04 | ESP32 | Observação |
|---|---|---|
| VCC | 5V | Alimentação do sensor |
| TRIG | GPIO 18 | Saída do ESP32 |
| ECHO | GPIO 19 | **Através de divisor resistivo 1 kΩ + 2 kΩ** |
| GND | GND | Terra comum |

### Divisor de tensão do ECHO

O HC-SR04 trabalha em 5 V. No robô físico, o sinal `ECHO` passa por um divisor para chegar ao ESP32 próximo de 3,3 V:

```text
HC-SR04 ECHO ---- 1 kΩ ----+---- GPIO 19 ESP32
                           |
                          2 kΩ
                           |
                          GND
```

O `diagram.json` já mostra esse divisor.

## Tabela dimensional dos componentes

As dimensões abaixo são as usadas para planejar a **base quadrada de 200 × 200 mm**. Para módulos genéricos existem pequenas diferenças entre fabricantes; por isso, antes de furar a placa definitiva, confira a peça comprada com régua ou paquímetro.

| Componente | Qtd. | Largura (mm) | Comprimento (mm) | Altura (mm) | Forma de fixação na base |
|---|---:|---:|---:|---:|---|
| Base do robô | 1 | 200 | 200 | 3 | Peça estrutural em acrílico/MDF/PVC |
| ESP32-DevKitC V4 | 1 | 27,94 | 48,26 | ~13* | Berço impresso, fita VHB/Velcro ou pequena placa suporte; manter USB acessível |
| TB6612FNG carrier | 1 | 15,2 | 20,3 | 1,8 PCB / ~12 com headers | Soldado em headers/perfboard ou preso em suporte/VHB |
| HC-SR04 | 1 | 45 | 20 | 15 | Suporte frontal em L; parafusos pequenos pelos furos do módulo ou abraçadeira |
| Motor TT 3–6 V | 2 | 22 | 70 | 18 | Suporte de motor TT em L + parafusos M3 |
| Roda TT 65 mm | 2 | 27 | 65 | 65 | Encaixe por pressão no eixo D do motor TT |
| Ball caster 1" | 1 | 34 | 34 | 29 | 3 parafusos M3; centralizado no eixo longitudinal |
| Suporte de 4 pilhas AA | 1 | 58 | 63 | 16 | Velcro/VHB ou duas abraçadeiras passando por rasgos na base |
| Regulador 5 V step-up/down S13V20F5 | 1 | 8,9 | 12,1 | 5,6 | Soldado em pequena perfboard ou preso com VHB |

\* A Espressif publica oficialmente o footprint da placa ESP32-DevKitC V4 como 48,26 × 27,94 mm. A altura de ~13 mm é uma **reserva mecânica prática** para a placa montada, pois varia conforme o módulo e os headers utilizados.

### Fontes dimensionais usadas

- ESP32-DevKitC V4: https://dl.espressif.com/dl/schematics/esp32_devkitc_v4_dimensions.pdf
- TB6612FNG carrier: https://www.pololu.com/file/0J1724/tb6612fng-dual-motor-driver-carrier-dimensions.pdf
- HC-SR04: https://www.mouser.com/datasheet/2/813/HCSR04-1022824.pdf
- Motor TT: https://www.mouser.com/datasheet/2/737/Adafruit_3777_Web-3326665.pdf
- Roda TT 65 mm: https://shop.4tronix.co.uk/products/65mm-yellow-wheel-for-robot-vehicle
- Ball caster: https://www.pololu.com/product/2692
- Suporte 4×AA: https://www.pololu.com/product/1153
- Regulador S13V20F5: https://www.pololu.com/product/4085/specs

## Distribuição na base de 200 × 200 mm

Sugestão de montagem física vista de cima:

```text
                        FRENTE
       ┌────────────────────────────────┐
       │         [ HC-SR04 ]            │
       │              ↑                 │
       │       área de detecção         │
       │                                │
  O====│ MOTOR E                 MOTOR D│====O
 RODA  │                                │  RODA
       │      ESP32       TB6612        │
       │                                │
       │       REG. 5V     BATERIA      │
       │                                │
       │             CASTER             │
       │               O                │
       └────────────────────────────────┘
                        TRASEIRA
```

### Posição recomendada do HC-SR04

- centralizado na frente;
- transdutores apontados horizontalmente para a direção de movimento;
- não deixar acrílico, roda ou cabo na frente dos dois transdutores;
- usar um pequeno suporte vertical em L;
- deixar o sensor suficientemente alto para não medir o próprio piso.

## Alimentação do robô físico

Sugestão de alimentação:

```text
4 x AA
  |
  +-------------------------------> VM do TB6612FNG
  |
  +--> regulador 5 V ------------> pino 5V do ESP32
  |                              -> VCC do HC-SR04
  |
 GND ----------------------------> GND comum
```

**Não alimente os motores pelos pinos 5V/3V3 do ESP32.**

O `VM` do TB6612FNG recebe a alimentação dos motores. O ESP32 recebe 5 V regulados. Todos os GNDs devem estar em comum.

## Segurança

O projeto possui duas proteções independentes:

### 1. Obstáculo frontal

Quando o HC-SR04 detecta algo a até 20 cm:

1. o ESP32 para imediatamente qualquer movimento;
2. bloqueia frente, esquerda e direita;
3. mantém apenas ré e stop disponíveis;
4. libera novamente quando a distância medida chegar a 25 cm ou mais.

### 2. Timeout de comando

Se o ESP32 estiver movimentando o robô e deixar de receber comandos por aproximadamente **900 ms**, os motores são parados.

Isso protege contra:

- queda do Wi-Fi;
- fechamento da página;
- perda de conexão do celular;
- botão/comando travado.

## Teste no Wokwi

O `diagram.json` contém:

- ESP32 DevKitC V4;
- HC-SR04 nativo do Wokwi;
- divisor resistivo do ECHO;
- TB6612FNG custom chip;
- dois conversores `stepper-esc`;
- dois motores visuais.

### Teste da proteção ultrassônica

1. inicie a simulação;
2. o HC-SR04 começa com distância de `400 cm`;
3. envie `F` no Serial Monitor;
4. clique no HC-SR04;
5. mova o controle de distância para, por exemplo, `15 cm`;
6. o robô deve parar e bloquear `F`, `L` e `R`;
7. envie `B` e confirme que a ré continua funcionando;
8. aumente a distância para `30 cm`;
9. os demais movimentos ficam liberados novamente.

O Wokwi permite alterar a distância simulada do HC-SR04 entre 2 cm e 400 cm.

## Estrutura do repositório

```text
robo-2wd-esp32/
├── sketch.ino
├── diagram.json
├── README.md
├── .gitignore
├── LICENSE
└── docs/
    ├── COMPONENTES.md
    ├── CONTROLE_WIFI.md
    ├── MONTAGEM.md
    ├── SENSOR_ULTRASSONICO.md
    └── WOKWI.md
```

## Documentação

- [Componentes](docs/COMPONENTES.md)
- [Montagem](docs/MONTAGEM.md)
- [Sensor ultrassônico](docs/SENSOR_ULTRASSONICO.md)
- [Controle Wi-Fi](docs/CONTROLE_WIFI.md)
- [Teste no Wokwi](docs/WOKWI.md)

## Referências técnicas

- Wokwi HC-SR04: https://docs.wokwi.com/parts/wokwi-hc-sr04
- Wokwi hardware suportado: https://docs.wokwi.com/getting-started/supported-hardware
- Wokwi TB6612FNG custom chip: https://github.com/drf5n/Wokwi-Chip-TB6612FNG
- Wokwi stepper-esc: https://github.com/drf5n/Wokwi-Chip-stepper-esc
- ESP32-DevKitC V4: https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32/esp32-devkitc/user_guide.html
