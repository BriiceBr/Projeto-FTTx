# Histórico de Desenvolvimento e Evolução Pedagógica da Bolsa

Este diretório registra a trajetória de aprendizado, desenvolvimento e amadurecimento científico durante a bolsa de estudos em redes FTTH no Colégio Técnico Industrial de Santa Maria (CTISM) / Universidade Federal de Santa Maria (UFSM).

---

## 📈 Fases do Desenvolvimento

```
[Fase 1: Fundamentos de AG & TSP]
       │
       ▼
[Fase 2: Primeiro Protótipo FTTx]
       │
       ▼
[Fase 3: Modelagem em Árvore de Steiner v1]
       │
       ▼
[Fase 4: AG Híbrido Final (Artigo ERRC 2026)] ──> /src/ag_ftth_hibrido.c
```

---

### 1. `01_aprendizado_ga_tsp/` - Fundamentos e Caixeiro Viajante
- **`intro_ag_hello_world/`**: Primeiros passos práticos com Algoritmos Genéticos em C, gerando uma frase alvo via mutação e recombinação.
- **`tsp_berlin52/`**: Implementação clássica de AG aplicado ao Caixeiro Viajante com a instância padronizada da TSPLIB (`berlin52.tsp`).
- **`tsp_4000_postes/`**: Teste de escalabilidade em malha de 4461 nós (`fnl4461.tsp`).
- **`simulador_tsp_20_postes/`**: Primeira tentativa de transpor o modelo de rotas para uma rede de 20 postes (`rede20.tsp`).

### 2. `02_primeiro_simulador_fttx/` - Transição para Instâncias de Redes Óticas
- **`Simulador_FTTx.c`**: Leitor do formato de instâncias do mestrado de Suzianne Tormann (UFSM, 2023) com separação de nós (OLT, Postes e Clientes).
- Constatação fundamental da pesquisa: redes de fibra ótica passiva (PON) **não podem ser modeladas como circuitos fechados (TSP)**, mas sim como árvores enraizadas na central (OLT).

### 3. `03_modelo_steiner_v1/` - Formulação em Árvore de Steiner
- **`Simulador_Steiner.c`**: Mudança conceitual para indivíduo representado por vetor de antecessores (`pai[TAM_MAPA]`).
- Introdução da heurística construtiva de Prim randomizada e do operador de poda iterativa de Steiner para eliminar postes sem clientes dependentes.
- Scripts auxiliares e visualizadores intermediários de testes.
