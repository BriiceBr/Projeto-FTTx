# Planejamento e Otimização de Redes FTTH Utilizando Algoritmos Genéticos

[![CI](https://github.com/BriiceBr/Projeto-FTTx/actions/workflows/ci.yml/badge.svg)](https://github.com/BriiceBr/Projeto-FTTx/actions/workflows/ci.yml)
[![Language: C](https://img.shields.io/badge/Language-C-00599C.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Python: 3.x](https://img.shields.io/badge/Python-3.x-3776AB.svg)](https://www.python.org/)
[![UFSM/CTISM](https://img.shields.io/badge/Institui%C3%A7%C3%A3o-UFSM%20%7C%20CTISM-red.svg)](https://www.ufsm.br/)
[![Conferência](https://img.shields.io/badge/Publica%C3%A7%C3%A3o-ERRC%202026-blue.svg)](https://errc.sbc.org.br/)

Repositório oficial do projeto de pesquisa desenvolvido na bolsa de estudos do **Colégio Técnico Industrial de Santa Maria (CTISM)** vinculado à **Universidade Federal de Santa Maria (UFSM)**, focado no planejamento físico, roteamento de cabos e minimização de custos de implantação em redes **FTTH (Fiber to the Home)** passivas (PON).

---

## 👥 Autores e Vinculação

- **Diogo Moi** ([diogo.moi@redes.ufsm.br](mailto:diogo.moi@redes.ufsm.br)) — *Autor Principal*
- **Viviane Kohler** ([viviane@redes.ufsm.br](mailto:viviane@redes.ufsm.br)) — *Orientadora*
- **Olinto de Araújo** ([olinto@ctism.ufsm.br](mailto:olinto@ctism.ufsm.br)) — *Coorientador*
- **Guilherme Dhein** ([gdhein@redes.ufsm.br](mailto:gdhein@redes.ufsm.br)) — *Coorientador*

---

## 📌 Visão Geral do Problema

A implantação de redes de banda larga em fibra ótica até a residência (FTTH) envolve elevados investimentos em infraestrutura física de postes urbanos. O problema de encontrar a rota ótima de cabos conectando uma **OLT (Optical Line Terminal)** central a todos os clientes finais (**ONUs**) através de postes intermediários viáveis modela-se matematicamente como uma variante da **Árvore de Steiner em Grafos (Steiner Minimal Tree - SMT)**:
- **Raiz:** OLT (ponto de transmissão ótica).
- **Nós de Steiner:** Postes de passagem disponíveis nas vias públicas.
- **Nós Terminais:** Assinantes/Clientes (que atuam estritamente como folhas terminais da árvore, não podendo servir como nós de trânsito).

Devido à natureza combinatória **NP-Difícil** do problema, métodos exatos de Programação Inteira Mista (MIP) tornam-se computacionalmente intratáveis para malhas reais acima de 64 clientes (Tormann, 2023). Este projeto propõe um **Algoritmo Genético Híbrido (AG Híbrido)** de alta performance implementado em linguagem C, capaz de convergir para soluções de alta qualidade em poucos segundos.

---

## 🧠 Arquitetura do Algoritmo Genético Híbrido

O resolvedor (`src/ag_ftth_hibrido.c`) incorpora heurísticas estruturais desenvolvidas especificamente para o problema ótico:

1. **Heurística Construtiva Guiada (Prim Randomizado):**
   - População inicial de 100 indivíduos gerada a partir da OLT por uma versão estocástica do algoritmo de Prim.
   - Sorteio por **loteria ponderada por posto** entre os $k$ melhores candidatos ($k \in [4, 8]$).
   - **Restrição rígida de integridade:** nós clientes são impedidos de se tornarem origem de novas ramificações.
2. **Poda Iterativa de Steiner:**
   - Varredura estrutural recursiva que identifica e elimina postes de grau unitário sem clientes dependentes, suprimindo trechos ociosos.
3. **Controle de Diversidade por Hashing:**
   - Cada topologia viável é indexada por um hash estrutural de 64 bits (`hash_individuo`), descartando duplicatas na população e preservando diversidade genética real.
4. **Cruzamento Estruturado em 3 Níveis:**
   - 70% de chance para arestas presentes em ambos os pais (herança conservadora).
   - 25% de chance para arestas presentes em apenas um dos pais.
   - 5% de chance para novas arestas viáveis da matriz de adjacência (reparo estrutural).
   - Herança em blocos de subárvores paternas (1 a 3 vértices conectados).
5. **Mutação em Dois Níveis com Fuga de Ótimos Locais:**
   - Mutação populacional (0% a 20%) e mutação individual por gene/vértice (1% a 5%).
   - Reconexão para antecessores viáveis com verificação de aciclicidade recursiva até a OLT.
6. **Métrica Geodésica Real:**
   - Distâncias entre coordenadas GPS calculadas diretamente via fórmula de **Haversine** (raio terrestre = 6.371.000 m).

---

## 📊 Principais Resultados Experimentais

Os experimentos foram conduzidos sobre coordenadas geográficas reais do município de **Nova Palma (RS)** e em malha sintética de controle:

### 1. Comparação de Seleção: Torneio ($k=3$) vs. Roleta Proporcional

| Instância | Clientes | Distância Torneio (m) | Distância Roleta (m) | Economia de Cabo (%) | Redução Absoluta |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **cidade64** (urbana) | 64 | **3.894,57** | 4.117,05 | **5,40%** | **222,48 m** |
| **interior64** (rural) | 64 | **6.799,44** | 6.948,12 | **2,14%** | **148,68 m** |
| **cidadepalma128** (ampla) | 128 | **20.195,16** | 20.393,72 | **0,97%** | **198,56 m** |
| **sintetica12** (controle) | 12 | **1.231,51** | 1.241,92 | **0,84%** | **10,41 m** |

> **Diagnóstico Teórico:** A seleção por Roleta baseada no inverso da distância sofre de perda de pressão seletiva em redes reais devido à alta magnitude métrica absoluta (as fatias da roleta tornam-se praticamente uniformes). O Torneio contorna a distorção por avaliar os indivíduos de forma estritamente ordinal.

### 2. Sintonia Fina das Taxas de Mutação

- **Redes Esparsas / Rurais (`sintetica12`, `interior64`):** Convergência ótima com taxas conservadoras (**5% populacional e 1% por gene**). O operador construtivo e o cruzamento são suficientes.
- **Redes Densas / Urbanas (`cidade64`):** Atingiu o melhor mínimo global (**3.852,42 m**) elevando a mutação por gene para **3%** (mantendo 5% populacional). Ambientes com muitas ruas sobrepostas necessitam de disrupção localizada nas subárvores para desarmar ótimos locais.

---

## 📁 Estrutura do Repositório

```text
Projeto-FTTx/
├── .github/
│   └── workflows/ci.yml            # Pipeline de compilação e teste no GitHub Actions
├── docs/                           # Documentação acadêmica e artigos
│   ├── Artigo_FTTH_ERRC2026_Moi.pdf# Artigo aceito/apresentado no ERRC 2026
│   ├── relatorios/                 # Relatórios técnicos de sintonia de parâmetros
│   ├── referencias/                # Dissertações de base (Tormann 2023, Oliveira 2019)
│   └── versoes_anteriores/         # Rascunhos preliminares do artigo
├── src/                            # Código-fonte principal do simulador
│   ├── ag_ftth_hibrido.c           # Algoritmo Genético Híbrido Final em C
│   ├── Makefile                    # Automação de compilação (Linux/macOS)
│   └── build.bat                   # Script de compilação para Windows (MinGW/GCC)
├── instancias/                     # Datasets das topologias de rede
│   ├── sintetica12.txt / toy3.txt  # Malha sintética de controle (Figura 1)
│   ├── cidade64.txt                # Nova Palma (RS) - Malha urbana
│   ├── interior64.txt              # Nova Palma (RS) - Malha rural
│   ├── novapalma128.txt            # Nova Palma (RS) - Malha completa (>20 km)
│   └── gerador_instancia.py        # Gerador sintético de malhas em grade
├── experimentos/                   # Baterias de testes do artigo
│   ├── selecao_torneio_vs_roleta/  # Código C e CSVs do comparativo de seleção
│   ├── varredura_mutacao/          # Código C e CSVs da varredura 2D de mutação
│   └── logs/                       # Logs detalhados de evolução
├── visualizador/                   # Ferramentas Python com NetworkX e Matplotlib
│   ├── plot_artigo_figura1.py      # Reproduz a Figura 1 do Artigo ERRC 2026
│   ├── verificacao_restricao_clientes.py # Script de validação da restrição de clientes
│   ├── visualizador_grafo.py       # Visualizador geral de topologias
│   └── comparativos/               # Gráficos comparativos de rotas
└── historico/                      # Evolução pedagógica da bolsa
    ├── 01_aprendizado_ga_tsp/      # Primeiros exercícios (Hello World, TSP berlin52, fnl4461)
    ├── 02_primeiro_simulador_fttx/ # Primeiro protótipo de leitura FTTx
    └── 03_modelo_steiner_v1/       # Primeira versão com Árvore de Steiner
```

---

## 🚀 Como Compilar e Executar

### Pré-requisitos
- Compilador **GCC** com suporte a C99/C11.
- **Python 3** com bibliotecas `matplotlib` e `networkx` (para visualização gráfica).

```bash
pip install matplotlib networkx
```

### Compilação

#### No Linux / macOS:
```bash
cd src
make
./ag_ftth_hibrido
```

#### No Windows (com MinGW / Code::Blocks):
```cmd
cd src
build.bat
ag_ftth_hibrido.exe
```
*Ou compile manualmente:*
```cmd
gcc -O3 ag_ftth_hibrido.c -o ag_ftth_hibrido.exe -lm
```

---

## 🎨 Visualização Gráfica das Redes

Para visualizar a topologia final gerada e plotada na **Figura 1** do artigo:

```bash
cd visualizador
python plot_artigo_figura1.py
# ou no Windows:
py plot_artigo_figura1.py
```

---

## 📚 Citação Bibliográfica

Caso utilize este código ou os datasets em sua pesquisa acadêmica, cite:

```bibtex
@inproceedings{moi2026ftth,
  author    = {Diogo Moi and Viviane Kohler and Olinto de Ara{\'u}jo and Guilherme Dhein},
  title     = {Planejamento e Otimiza{\c{c}}{\~a}o de Redes FTTH Utilizando Algoritmos Gen{\'e}ticos},
  booktitle = {Anais da Escola Regional de Redes de Computadores (ERRC 2026)},
  year      = {2026},
  address   = {Santa Maria, RS, Brasil},
  publisher = {Sociedade Brasileira de Computa{\c{c}}{\~a}o (SBC)}
}
```

---

## 📜 Licença e Agradecimentos

Este projeto foi desenvolvido com apoio e recursos do **CTISM / Universidade Federal de Santa Maria (UFSM)** no âmbito da bolsa de iniciação científica e tecnológica em redes de telecomunicações.
