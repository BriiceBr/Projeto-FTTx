import networkx as nx
import matplotlib.pyplot as plt
import os

# 1. Cria a rede
G = nx.DiGraph()

# 2. Descobre a pasta onde o Visualizador.py esta e junta com o nome do txt
pasta_atual = os.path.dirname(os.path.abspath(__file__))
caminho_txt = os.path.join(pasta_atual, 'toy3.txt')

posicoes_reais = {}
lendo_nos = False

# 3. Abre o txt usando o caminho completo e a prova de falhas
with open(caminho_txt, 'r') as arq:
    for linha in arq:
        if 'Nodes' in linha:
            lendo_nos = True
            continue
        if 'Arcs' in linha or 'Splitters' in linha:
            lendo_nos = False
            
        if lendo_nos:
            partes = linha.split()
            if len(partes) >= 3:
                id_no = int(partes[0])
                x = float(partes[1])
                y = float(partes[2])
                posicoes_reais[id_no] = (x, y) # Guarda a posicao exata no mapa

# 4. Adiciona as conexoes exatas (Individuo 0 atual)
conexoes = [
    (1, 2), (9, 3), (10, 9), (11, 10), (26, 11), (11, 12),
    (12, 13), (22, 21), (2, 22), (2, 26), (45, 29), (29, 30),
    (30, 31), (31, 32), (3, 41), (32, 42), (32, 43), (10, 44),
    (29, 45), (12, 46), (13, 47), (21, 48), (22, 49), (30, 50),
    (29, 51), (3, 52)
]
G.add_edges_from(conexoes)

# 5. Desenha a rede usando as posicoes geograficas do arquivo
plt.figure(figsize=(12, 8))

cores = []
for node in G.nodes():
    if node == 1:
        cores.append('lightgreen') # OLT
    elif node >= 41:
        cores.append('salmon') # Clientes
    else:
        cores.append('lightblue') # Postes de passagem

# Usa o "pos=posicoes_reais" para desenhar linhas retas como quarteiroes reais
nx.draw(G, pos=posicoes_reais, with_labels=True, node_color=cores, node_size=600, font_weight='bold', arrows=True)
plt.title("Geografia Real da Rede FTTx")
plt.grid(True, linestyle='--', alpha=0.6) # Adiciona linhas de grade de fundo
plt.show()