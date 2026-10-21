# Dataset — CP2 TinyML

## Sensor utilizado
- Sensor:
- Microcontrolador utilizado na coleta:
- Grandeza medida:
- Unidade de medida:

## Método de coleta
Descrever como as medições foram realizadas, as condições de coleta e como cada arquivo CSV foi gerado.

## Classes do modelo

| Classe | Significado |
|---|---|
| `atencao` | Descrever a condição real |
| `livre` | Descrever a condição real |
| `perigo` | Descrever a condição real |

## Arquivos coletados

Os arquivos CSV estão organizados nesta pasta, separados por classe.

- `atencao_001.csv` até `atencao_010.csv`
- `livre_001.csv` até `livre_010.csv`
- `perigo_001.csv` até `perigo_010.csv`

Total: 30 arquivos CSV.

**Observação:** a quantidade de arquivos não representa necessariamente a quantidade total de amostras. O total de medições deve ser contabilizado a partir dos dados reais.

## Estrutura dos dados
Descrever os nomes das colunas, os tipos de dados, as unidades e se existe uma coluna de rótulo.

## Preparação para o Edge Impulse
Descrever como os arquivos foram importados e como os dados foram organizados para treinamento, validação e teste.

## Limitações
Registrar eventuais problemas de coleta, desequilíbrio entre classes ou limitações do dataset.