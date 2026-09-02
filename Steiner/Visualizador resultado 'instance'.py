import networkx as nx
import matplotlib.pyplot as plt
import os

# 1. Cria a rede
G = nx.DiGraph()

# 2. Descobre a pasta onde o Visualizador.py esta e junta com o nome do txt
pasta_atual = os.path.dirname(os.path.abspath(__file__))

# ATENCAO 1: Substitua pelo nome real do novo arquivo (ex: 'toy1.txt')
caminho_txt = os.path.join(pasta_atual, 'instance.txt')

posicoes_reais = {}
lendo_nos = False

# 3. Abre o txt usando o caminho completo
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
                posicoes_reais[id_no] = (x, y)

# 4. Adiciona as conexoes exatas (Custo: 90.07 metros)
conexoes = [
    (1, 2), (8, 3), (3, 4), (4, 5), (5, 6), (6, 7), (2, 8),
    (7, 9), (8, 10), (9, 11), (10, 12), (11, 13), (12, 14),
    (14, 15), (15, 16), (16, 17), (13, 18), (2, 19), (8, 20),
    (8, 21), (14, 22), (16, 23), (9, 24), (9, 25), (9, 26),
    (6, 27), (4, 28)
]
G.add_edges_from(conexoes)

# 5. Desenha a rede usando as posicoes geograficas do arquivo
plt.figure(figsize=(12, 8))

cores = []

# ATENCAO 2: Coloque aqui o ID do primeiro cliente desta instancia
ID_INICIO_CLIENTES = 18 

for node in G.nodes():
    if node == 1:
        cores.append('lightgreen') # OLT
    elif node >= ID_INICIO_CLIENTES:
        cores.append('salmon') # Clientes
    else:
        cores.append('lightblue') # Postes de passagem

# Usa o pos=posicoes_reais para desenhar as ruas
nx.draw(G, pos=posicoes_reais, with_labels=True, node_color=cores, node_size=600, font_weight='bold', arrows=True)
plt.title("Rede FTTx (Nova Instância) - Custo: 90.07m")
plt.grid(True, linestyle='--', alpha=0.6)
plt.show()