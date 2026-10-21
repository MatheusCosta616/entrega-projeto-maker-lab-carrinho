## Integrantes

| Nome | RM |
|---|---|
| Caique Walter | 550693 |
| Guilherme Espim | 99499 |
| Guilherme Nobre | 98604 |
| João Paulo Zamperlinni | 99279 |
| Matheus José | 551157 |


# Carrinho-robô ESP32 com TinyML

Projeto da disciplina **Project-based Maker Lab**, do curso de Engenharia de Software da FIAP.

Carrinho-robô 2WD controlado por ESP32 via Wi-Fi, equipado com sensor ultrassônico HC-SR04. No CP2, o projeto incorpora um modelo TinyML treinado no Edge Impulse para classificar as condições à frente do carrinho (`livre`, `atencao` e `perigo`) e utilizar essa classificação na tomada de decisão do firmware.

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
| Firmware do CP1 | [`src/cp1/carrinho.ino`](src/cp1/carrinho.ino) |
| Firmware do CP2 com TinyML | [`src/cp2/carrinho-cp2.ino`](src/cp2/carrinho-cp2.ino) |
| Sketch de coleta de dados | [`src/coleta-esp/coleta-esp.ino`](src/coleta-esp/coleta-esp.ino) |
| Dataset de treinamento | [`dados/`](dados/) |
| Biblioteca exportada do Edge Impulse | [`model/biblioteca-edge-impulse/`](model/biblioteca-edge-impulse/) |
| Documentação do TinyML | [`docs/tinyml.md`](docs/tinyml.md) |
| Documentação de hardware | [`docs/hardware.md`](docs/hardware.md) |
| Evidências do projeto | [`docs/evidencias/`](docs/evidencias/) |
| Vídeo de demonstração do CP1 | [`midia/demo-cp1.mp4`](midia/demo-cp1.mp4) |
| Gestão do projeto | [`organizacao/`](organizacao/) |
| Simulação do MVP no Wokwi | [`organizacao/MVP/README.md`](organizacao/MVP/README.md) |

## Estrutura do repositório

```text
.
├── README.md
├── integrantes.txt
├── dados/
│   ├── README.md
│   ├── atencao_001.csv ... atencao_010.csv
│   ├── livre_001.csv ... livre_010.csv
│   └── perigo_001.csv ... perigo_010.csv
├── docs/
│   ├── hardware.md
│   ├── tinyml.md
│   └── evidencias/
│       └── edge-impulse/
├── model/
│   └── biblioteca-edge-impulse/
│       └── ei-guiespim18-project-1-arduino-1.0.3-impulse-#1.zip
├── src/
│   ├── carrinho/
│   │   └── carrinho.ino
│   ├── carrinho-cp2/
│   │   └── carrinho-cp2.ino
│   └── coleta-esp/
│       └── coleta-esp.ino
├── midia/
│   └── demo-cp1.mp4
└── organizacao/
    ├── Backlog/
    ├── Custos/
    ├── Kanban/
    ├── MoSCoW/
    └── MVP/
```

*Observação: a estrutura acima representa os arquivos do repositório no momento da revisão. Atualize-a caso novos arquivos ou pastas sejam adicionados.*

## Dataset

O diretório [`dados/`](dados/) contém 30 arquivos CSV organizados em três classes:

- `livre`: condição de caminho livre.
- `atencao`: condição intermediária que requer atenção.
- `perigo`: condição de proximidade de obstáculo.

Há 10 arquivos por classe na estrutura atual. A documentação em [`dados/README.md`](dados/README.md) deve descrever o formato dos dados, o sensor utilizado e o procedimento de coleta.

## Modelo TinyML

O modelo foi desenvolvido no Edge Impulse a partir dos dados coletados com o sensor ultrassônico HC-SR04.

A biblioteca Arduino exportada está armazenada em [`model/biblioteca-edge-impulse/`](model/biblioteca-edge-impulse/).

A documentação do ciclo TinyML — coleta, treinamento, avaliação, exportação, inferência embarcada e integração com o firmware — está em [`docs/tinyml.md`](docs/tinyml.md).

As evidências disponíveis estão em [`docs/evidencias/`](docs/evidencias/).

## Como executar o CP2

1. Instale a Arduino IDE 2.x.
2. Instale o pacote de placas ESP32 da Espressif pelo Gerenciador de Placas.
3. Importe a biblioteca ZIP exportada pelo Edge Impulse usando **Sketch → Include Library → Add .ZIP Library**.
4. Abra [`src/cp2/carrinho-cp2.ino`](src/cp2/carrinho-cp2.ino).
5. Selecione a placa correspondente ao ESP32 utilizado, como **ESP32 Dev Module**, e a porta serial correta.
6. Confira as configurações de Wi-Fi e os pinos definidos no firmware antes do envio.
7. Compile e envie o programa para o ESP32.
8. Teste primeiro com as rodas suspensas e confirme o comportamento do carrinho antes de utilizá-lo no chão.

Os comandos Wi-Fi, endereço de acesso e credenciais dependem da configuração efetiva do firmware. Consulte o código e a documentação antes de operar o carrinho.

## Hardware

O projeto utiliza um chassi 2WD, motores DC, driver de motores, ESP32 e sensor ultrassônico HC-SR04.

A pinagem, a alimentação elétrica, as conexões e os cuidados de montagem estão descritos em [`docs/hardware.md`](docs/hardware.md).

## Evidências

As evidências do treinamento e dos testes devem ser consultadas em [`docs/evidencias/`](docs/evidencias/).

O arquivo [`midia/demo-cp1.mp4`](midia/demo-cp1.mp4) corresponde à demonstração do CP1 e documenta uma etapa anterior do projeto. Ele não deve ser interpretado, isoladamente, como comprovação da integração TinyML do CP2.

## Evolução do projeto

O projeto é contínuo: cada checkpoint evolui o carrinho e seus componentes de software.

| Checkpoint | Data | Entrega |
|---|---|---|
| CP1 | 16/09/2026 | Carrinho físico com controle Wi-Fi e comportamento de parada por obstáculo. |
| CP2 | 21/10/2026 | Integração de modelo TinyML treinado no Edge Impulse ao firmware do carrinho. |

O histórico de commits registra a evolução do repositório ao longo dos checkpoints.

## Organização do projeto

A pasta [`organizacao/`](organizacao/) contém os artefatos de gestão, incluindo MVP, backlog, Kanban, MoSCoW e planilha de custos.

## Licença e utilização

Consulte as licenças dos componentes e bibliotecas utilizados. Este repositório reúne os artefatos desenvolvidos para fins acadêmicos na disciplina Project-based Maker Lab.
