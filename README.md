# Carrinho-robô ESP32 com TinyML

Projeto da disciplina **Project-based Maker Lab** (FIAP, Engenharia de Software). Carrinho-robô 2WD controlado por ESP32 via Wi-Fi, com sensor ultrassônico HC-SR04 e um modelo TinyML treinado no Edge Impulse que classifica a distância à frente (`livre`, `atencao`, `perigo`) e ajusta a velocidade do carrinho em tempo real.

## Integrantes

| Nome | RM |
|---|---|
| Caique Walter | 550693 |
| Guilherme Espim | 99499 |
| Guilherme Nobre | 98604 |
| João Paulo Zamperlinni | 99279 |
| Matheus José | 551157 |

## Onde está cada coisa

| Conteúdo | Local |
|---|---|
| Firmware do carrinho (controle Wi-Fi + inferência TinyML) | [`src/carrinho/carrinho.ino`](src/carrinho/carrinho.ino) |
| Sketch de coleta de dados do sensor | [`src/coleta-esp/coleta-esp.ino`](src/coleta-esp/coleta-esp.ino) |
| Dataset (CSVs coletados com o HC-SR04) | [`dados/`](dados/) |
| Biblioteca exportada do Edge Impulse | [`model/`](model/) |
| **Documentação do CP2 (TinyML)** | [`docs/tinyml.md`](docs/tinyml.md) |
| Evidências do Edge Impulse e dos testes | [`docs/evidencias/`](docs/evidencias/) |
| Hardware, pinagem, alimentação e mecânica | [`docs/hardware.md`](docs/hardware.md) |
| Vídeo da demonstração do CP1 | [`midia/demo-cp1.mp4`](midia/demo-cp1.mp4) |
| Gestão: MVP, MoSCoW, backlog, kanban, custos | [`organizacao/`](organizacao/) |
| Simulação no Wokwi (MVP) | [`organizacao/MVP/`](organizacao/MVP/README.md) |

## Estrutura do repositório

```text
.
├── src/
│   ├── carrinho/carrinho.ino      # firmware do carrinho (evoluído do CP1 para o CP2)
│   └── coleta-esp/coleta-esp.ino  # coleta de dados para o dataset
├── dados/                         # 9 CSVs (3 por classe)
├── model/                         # biblioteca Arduino gerada pelo Edge Impulse
├── docs/
│   ├── tinyml.md                  # ciclo coletar → treinar → testar → embarcar → inferir → agir
│   ├── hardware.md                # eletrônica, alimentação e mecânica
│   └── evidencias/                # prints do Edge Impulse e dos testes
├── midia/                         # vídeos de demonstração
└── organizacao/                   # gestão do projeto
```

## Como executar

1. Instale a Arduino IDE 2.x e o pacote **esp32 by Espressif** (core 2.0 ou superior, necessário para `analogWrite`).
2. Importe a biblioteca exportada pelo Edge Impulse: *Sketch → Include Library → Add .ZIP Library* com o arquivo de [`model/`](model/).
3. Abra [`src/carrinho/carrinho.ino`](src/carrinho/carrinho.ino), selecione a placa **ESP32 Dev Module** e faça o upload.
4. Conecte o celular à rede `ESP32_RC` (senha `esp32pass`) e acesse `http://192.168.4.1`.
5. A página mostra os comandos de direção e, a cada segundo, a classe inferida, a distância e o PWM aplicado.

## Evolução do projeto

O projeto é contínuo: cada checkpoint evolui o mesmo carrinho e o mesmo firmware, e o histórico de commits registra essa evolução.

| Checkpoint | Data | Entrega |
|---|---|---|
| CP1 | 16/09/2026 | carrinho físico com controle Wi-Fi e parada fixa por obstáculo a 30 cm |
| CP2 | 21/10/2026 | modelo TinyML embarcado classificando a distância e controlando a velocidade do carrinho ([detalhes](docs/tinyml.md)) |

Depois do CP1 o repositório ganhou este README, a documentação de hardware e mecânica em [`docs/hardware.md`](docs/hardware.md) e a documentação do TinyML em [`docs/tinyml.md`](docs/tinyml.md).
