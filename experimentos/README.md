# Baterias de Experimentos e Resultados (ERRC 2026)

Este diretório contém os códigos em C utilizados para executar as baterias experimentais descritas na seção **4. Resultados e Discussão** do artigo **"Planejamento e Otimização de Redes FTTH Utilizando Algoritmos Genéticos"**, bem como os dados brutos e planilhas compiladas.

---

## 🔬 Subdiretórios

### 1. `selecao_torneio_vs_roleta/` (Seção 4.1 do Artigo)
- **`ag_ftth_testes_roleta_x_torneio.c`**:
  Programa em C que executa ambos os operadores de seleção (Torneio k=3 e Roleta proporcional ao inverso da distância) sob congelamento da mesma semente pseudoaleatória, garantindo que ambos iniciem com populações idênticas.
- **`resultados_comparativo.csv`**:
  Resultados consolidados que compõem a **Tabela 1** do artigo:
  - `cidade64`: Torneio alcançou 3.894,57 m vs. Roleta 4.117,05 m (**redução de 5,40%**).
  - `cidadepalma128`: Torneio gerou economia absoluta de ~198 metros de cabo de fibra ótica.
- **`Tabela_Resultado_Testes.xlsx`**:
  Planilha com consolidação detalhada dos tempos computacionais e convergência.

### 2. `varredura_mutacao/` (Seção 4.2 do Artigo)
- **`teste_taxas_mutacao.c`**:
  Executa uma varredura bidimensional cruzando:
  - Probabilidade de mutação da população: 0% a 20%.
  - Probabilidade individual de alteração por gene: 1% a 5%.
- **`varredura_mutacao_*.csv`**:
  Tabelas de varredura correspondentes à **Tabela 2** do artigo para cada instância (`cidade64`, `interior64`, `novapalma128`, `toy3`).

### 3. `logs/`
- Arquivos de log gerados durante as execuções contendo histórico geracional, distância mínima, média, diversidade genética e mapeamento completo das conexões pai-filho da elite.
