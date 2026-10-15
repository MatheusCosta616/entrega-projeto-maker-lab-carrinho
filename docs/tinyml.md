# TinyML embarcado no carrinho (CP2)

O carrinho usa um modelo treinado no Edge Impulse com dados do próprio sensor ultrassônico para classificar a situação à frente e decidir a velocidade dos motores. O ciclo completo está documentado abaixo na ordem em que foi executado.

```text
COLETAR → TREINAR → TESTAR → EMBARCAR → INFERIR → AGIR
```

## Evolução em relação ao CP1

| Aspecto | CP1 | CP2 |
|---|---|---|
| Uso do sensor | regra fixa: para a 30 cm | entrada do modelo TinyML (janela de 20 leituras) e parada fixa mantida como segurança |
| Decisão | `if` com limiar de distância | classificação `livre` / `atencao` / `perigo` no ESP32 |
| Motores | liga/desliga (`digitalWrite`) | velocidade por PWM conforme a classe |
| Interface web | comandos de direção | comandos + classe, confiança, distância e PWM em tempo real |

O firmware é o mesmo sketch nos dois checkpoints ([`src/carrinho/carrinho.ino`](../src/carrinho/carrinho.ino)); a versão do CP1 continua acessível pelo histórico do git (`git log --follow src/carrinho/carrinho.ino`).

## 1. Coletar

**Sketch:** [`src/coleta-esp/coleta-esp.ino`](../src/coleta-esp/coleta-esp.ino)

O sketch roda no ESP32 do carrinho, com o HC-SR04 na mesma posição de uso (TRIG no GPIO 5, ECHO no GPIO 2). Ao receber `c` pelo Serial Monitor, ele faz 21 leituras com intervalo de 100 ms (janela de 2 s) e imprime no formato CSV `timestamp,distance_cm`. Leituras sem eco retornam `-1`.

Cada saída foi salva como um arquivo em [`dados/`](../dados/), com o nome no padrão `<classe>_<nnn>.csv`, que o Edge Impulse usa para inferir o rótulo.

| Classe | Critério de coleta | Arquivos | Faixa medida (mediana) |
|---|---|---|---|
| `livre` | caminho desobstruído | `livre_001..003` | 60,4 / 71,4 / 80,1 cm |
| `atencao` | obstáculo a média distância | `atencao_001..003` | 35,3 / 44,8 / 45,0 cm |
| `perigo` | obstáculo próximo | `perigo_001..003` | 15,5 / 20,6 / 25,8 cm |

Total: 9 amostras de 21 leituras cada (189 leituras).

## 2. Treinar

Projeto no Edge Impulse: **CP2_TinyML_ESP32** (nome que gera a biblioteca `CP2_TinyML_ESP32_inferencing.h`).

| Etapa | Configuração |
|---|---|
| Data acquisition | upload dos 9 CSVs pelo CSV Wizard, rótulo pelo nome do arquivo |
| Divisão dos dados | treino/teste *(preencher a proporção usada, ex.: 80/20, e quais amostras ficaram no teste)* |
| Time series data | 1 eixo (`distance_cm`), 10 Hz, janela de 20 leituras (≈2 s, conforme o firmware) |
| Processing block | Raw Data *(confirmar no projeto)* |
| Learning block | Classification (3 classes) |

> **Pendente (grupo):** confirmar os blocos acima e preencher com o link público do projeto, a acurácia, o loss e a matriz de confusão exibidos na aba *Classifier*, e salvar o print em [`docs/evidencias/`](evidencias/).

| Métrica | Valor |
|---|---|
| Link público | |
| Acurácia (validação) | |
| Loss | |
| Modelo | Quantized (int8) / float32 |

## 3. Testar

> **Pendente (grupo):** print da aba *Model testing* em [`docs/evidencias/`](evidencias/) e registro do teste físico (objeto aproximado a cada faixa, 5 repetições por classe, classe observada na página web).

## 4. Embarcar

Em *Deployment*, o modelo foi exportado como **Arduino Library** e instalado na Arduino IDE via *Add .ZIP Library*. O `.zip` exportado fica em [`model/`](../model/) para que qualquer pessoa consiga compilar o firmware sem acesso à conta do Edge Impulse.

## 5. Inferir

**Firmware:** [`src/carrinho/carrinho.ino`](../src/carrinho/carrinho.ino)

A cada `EI_CLASSIFIER_INTERVAL_MS` o firmware lê o HC-SR04 e guarda a leitura num buffer. Quando o buffer atinge `EI_CLASSIFIER_RAW_SAMPLE_COUNT` leituras (a mesma janela usada no treino), chama `run_classifier()` e fica com a classe de maior probabilidade. Leituras inválidas (`-1`) zeram o buffer em vez de entrar no modelo, porque um valor que nunca apareceu no treino produziria uma classificação sem sentido.

## 6. Agir

A classe inferida muda o comportamento do carrinho por meio do PWM dos pinos ENA/ENB da ponte H:

| Classe inferida | Confiança | Ação no carrinho |
|---|---|---|
| `livre` | ≥ 60% | velocidade normal (PWM 255) |
| `atencao` | qualquer | velocidade reduzida (PWM 140) |
| qualquer classe | < 60% | velocidade reduzida (PWM 140), por precaução |
| `perigo` | ≥ 60% | para os motores e bloqueia o comando de frente até a classe mudar |

Ré e curvas continuam liberadas em todas as classes, para o carrinho poder sair da situação de perigo. A página web consulta a rota `/status` a cada segundo e mostra classe, confiança, distância e PWM, o que permite ver a decisão do modelo durante a demonstração.

Existe ainda uma parada de segurança fixa a 30 cm, independente do modelo. Ela é a camada de proteção caso o modelo erre ou a janela ainda não esteja completa; o comportamento graduado (reduzir velocidade antes de parar) vem exclusivamente do TinyML.

## Limitações conhecidas

O dataset tem 3 amostras por classe, todas com o obstáculo parado. Isso basta para separar as faixas de distância, mas deixa o modelo frágil para objetos em movimento e para as fronteiras entre classes (por exemplo, `perigo_003` a 25,8 cm e `atencao_003` a 35,3 cm). O próximo passo seria coletar pelo menos 10 amostras por classe, incluindo aproximações com o carrinho em movimento.
