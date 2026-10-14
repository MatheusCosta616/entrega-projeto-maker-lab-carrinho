# Arquitetura Eletrônica do Carrinho Robô

## Visão Geral

Este projeto consiste em um carrinho robô controlado por um microcontrolador (Arduino ou ESP32), responsável por receber dados dos sensores, processá-los e controlar os motores através de uma ponte H. O sistema também possui comunicação Bluetooth para interação remota e uma estrutura de alimentação dedicada para garantir o funcionamento adequado de todos os componentes.

---

# Diagrama de Arquitetura

```text
                    +------------------+
                    | Sensor           |
                    | Ultrassônico     |
                    +--------+---------+
                             |
                             v
                    +------------------+
                    | Arduino / ESP32  |
                    | Controle Central |
                    +--------+---------+
                             ^
                             |
                    +--------+---------+
                    | Bluetooth        |
                    | Comunicação      |
                    +------------------+

                             |
                   Sinais de Controle
                             |
                             v
                    +------------------+
                    | Ponte H          |
                    | Driver de Motor  |
                    +----+--------+----+
                         |        |
                         v        v
                  +---------+ +---------+
                  | Motor A | | Motor B |
                  +---------+ +---------+
```

---

# Diagrama de Alimentação

```text
                    +-----------+
                    | Bateria   |
                    +-----+-----+
                          |
                    +-----+-----+
                    | Chave Liga|
                    | / Desliga |
                    +-----+-----+
                          |
             +------------+------------+
             |                         |
             v                         v

       +-----------+           +---------------+
       | Ponte H   |           | Regulador de  |
       | Motores   |           | Tensão        |
       +-----+-----+           +-------+-------+
             |                         |
             v                         v

       +-----------+           +---------------+
       | Motores   |           | Arduino/ESP32 |
       +-----------+           | Bluetooth     |
                               | Sensor        |
                               +---------------+
```

---

# Componentes

## Arduino ou ESP32
Controlador principal do robô, responsável pelo processamento das informações e controle dos periféricos.

## Sensor Ultrassônico
Utilizado para medir distâncias e detectar obstáculos.

## Módulo Bluetooth
Permite comunicação sem fio com celular ou computador.

## Ponte H
Driver responsável pelo acionamento e controle dos motores.

## Motores DC
Atuadores que realizam a movimentação do carrinho.

## Bateria
Fonte principal de alimentação do sistema.

## Regulador de Tensão
Adequa a tensão da bateria para os componentes eletrônicos.

## Chave Liga/Desliga
Controla o fornecimento de energia para o sistema.

---

# Fluxo de Funcionamento

1. O usuário envia comandos via Bluetooth ou o sensor detecta obstáculos.
2. O Arduino/ESP32 processa os dados recebidos.
3. Os comandos são enviados para a Ponte H.
4. A Ponte H controla os motores.
5. Os motores movimentam o carrinho robô.

---

# Resumo da Arquitetura

| Componente | Função |
|------------|---------|
| Bateria | Fornecer energia ao sistema |
| Chave Liga/Desliga | Controlar alimentação |
| Regulador de Tensão | Ajustar tensão dos circuitos |
| Arduino/ESP32 | Processamento e controle |
| Sensor Ultrassônico | Detecção de obstáculos |
| Bluetooth | Comunicação sem fio |
| Ponte H | Acionamento dos motores |
| Motores DC | Movimentação do robô |
