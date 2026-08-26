# MVP — Carrinho-robô

## Projeto
Carrinho-robô com 4 rodas motorizadas, controlado via Wi-Fi/app, com módulo Bluetooth como recurso adicional. Carenagem final impressa em 3D.

---

## Quando estará minimamente funcional (MVP)

O MVP é atingido quando o carrinho **anda de forma controlada**, sem carenagem final e sem os recursos extras. Ele deve:

- [ ] Ter o chassi (mesmo que provisório) com as 4 rodas montadas
- [ ] Ter os 4 motores instalados e funcionando
- [ ] Ter a ponte H conectada corretamente aos motores e ao microcontrolador
- [ ] Ter o Arduino/ESP32 alimentado por bateria (ou power bank)
- [ ] Executar os comandos básicos: **avançar, recuar, virar à esquerda, virar à direita, parar**
- [ ] Receber comandos via Wi-Fi (rede local, servidor simples embutido no ESP32) através de um app/página de controle básica (botões: forward / reverse / left / right / stop)

> Critério de aceite do MVP: o carrinho deve se mover nas 4 direções de forma controlada e repetível, mesmo que ainda esteja "com os fios para fora" e sem carenagem.

---

## Quando estará concluído (Produto Final)

O projeto será considerado concluído quando, além de tudo o que está no MVP, o carrinho tiver:

- [ ] Carenagem final impressa em 3D, com todos os componentes protegidos e fixados internamente
- [ ] Módulo Bluetooth funcional como canal de controle alternativo ao Wi-Fi
- [ ] Testes de autonomia de bateria documentados
- [ ] Documentação completa no GitHub (MVP, MoSCoW, Backlog, Kanban, Planilha de custos)
- [ ] Vídeo/demo do funcionamento completo (Wi-Fi + Bluetooth + movimentação nas 4 direções)

---

## Fora de escopo nesta versão
- Sensor ultrassônico / desvio de obstáculos
- Modo autônomo
- LEDs RGB, buzzer, RFID
- App mobile dedicado (fica só web/página simples)
