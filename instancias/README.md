# Instâncias de Rede FTTH / Topologias de Teste

Este diretório centraliza todas as malhas de rede utilizadas na pesquisa e nos experimentos do artigo **"Planejamento e Otimização de Redes FTTH Utilizando Algoritmos Genéticos" (ERRC 2026)**.

---

## 📋 Formato dos Arquivos de Instância

Os arquivos seguem a modelagem padrão derivada da literatura de planejamento físico de redes PON/FTTx:
- `Clients <N>`: Quantidade de nós clientes (ONUs).
- `Nodes <M>`: Quantidade total de nós da infraestrutura (OLT + Postes de passagem + Clientes).
- Seção de coordenadas: `<id> <coord_y> <coord_x>` (onde para dados GPS, representam Latitude e Longitude; para malhas sintéticas, coordenadas cartesianas 2D).
- `Arcs <A>`: Número de vias físicas viáveis entre postes/estruturas.
- Pares de arcos: `<origem> <destino>`.

> **Nota sobre tipos de nós:**
> - `Nó 1`: Raiz da rede (OLT - Optical Line Terminal).
> - `Nós intermediários`: Infraestrutura física de postes de passagem (nós de Steiner).
> - `Últimos nós`: Clientes finais (ONUs). Clientes atuam estritamente como nós folha terminais (nunca podem ser nós de passagem/trânsito de cabos).

---

## 🗺️ Catálogo de Instâncias

| Arquivo | Descrição | Nós Totais | Clientes | Tipo de Distância | Papel no Artigo |
| :--- | :--- | :---: | :---: | :---: | :--- |
| `sintetica12.txt` / `toy3.txt` | Malha sintética reduzida para controle | 52 | 12 | Cartesiana (Pitágoras) | Figura 1, Tabela 1 e Tabela 2 |
| `cidade64.txt` | Malha urbana de Nova Palma (RS) | 175 | 64 | Geográfica (Haversine) | Tabela 1 e Tabela 2 |
| `interior64.txt` | Malha rural do município de Nova Palma (RS) | 175 | 64 | Geográfica (Haversine) | Tabela 1 e Tabela 2 |
| `interior64b.txt` | Variante da malha rural de Nova Palma | 175 | 64 | Geográfica (Haversine) | Testes comparativos |
| `novapalma128.txt` | Topologia em larga escala de Nova Palma (>20 km) | 367 | 128 | Geográfica (Haversine) | Tabela 1 e Tabela 2 |
| `novapalma_oltcidade.txt` | Variante com OLT reposicionada no centro urbano | 367 | 128 | Geográfica (Haversine) | Estudo de alocação de OLT |
| `cidade_teste.txt` | Grade retangular sintética | 126 | 25 | Cartesiana (Pitágoras) | Testes estruturais |
| `instance.txt` | Malha auxiliar de bancada | - | - | Cartesiana | Depuração de código |
| `y2.txt` | Topologia auxiliar reduzida | - | - | Cartesiana | Testes de conectividade |

---

## 🛠️ Gerador de Instâncias

- `gerador_instancia.py`: Script em Python que gera automaticamente malhas regulares em grade bidimensional com conectividade de vias horizontais e verticais em mão dupla.
