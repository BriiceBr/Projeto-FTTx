# Visualizadores de Topologias e Redes Óticas FTTx

Este diretório contém os scripts desenvolvidos em Python (utilizando `NetworkX` e `Matplotlib`) para representação visual, validação topológica e geração dos gráficos apresentados no artigo **ERRC 2026**.

---

## 📂 Estrutura de Arquivos

- **`plot_artigo_figura1.py`**:
  Gera exatamente a **Figura 1** do artigo ("Topologia de rede otimizada obtida pelo Algoritmo Genético sintetica12"), formatada em escala de cinza conforme as diretrizes de publicação SBC.
- **`verificacao_restricao_clientes.py`**:
  Script que demonstra a evolução da restrição topológica que impede clientes de atuarem como nós de passagem. Mostra o traçado antes e depois da correção da conectividade física.
- **`visualizador_grafo.py`**:
  Visualizador geral configurável para inspecionar árvores de conexão sobre qualquer arquivo de instância.
- **`comparativos/`**:
  Conjunto de scripts individuais que desenham os melhores traçados encontrados por método (Torneio vs. Roleta) para as instâncias `cidade64`, `interior64`, `interior64b` e `novapalma128`.
- **`imagens/`**:
  Armazena as imagens renderizadas das redes (`grafo_otimizado.png`, `cidade64_verificacao.png`, etc.).

---

## 🚀 Como Executar

Certifique-se de que possui as bibliotecas necessárias instaladas:
```bash
pip install matplotlib networkx
```

Para gerar a Figura 1 do artigo:
```bash
python plot_artigo_figura1.py
# ou no Windows:
py plot_artigo_figura1.py
```
