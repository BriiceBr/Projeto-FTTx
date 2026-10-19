import networkx as nx
import matplotlib.pyplot as plt
import os

# 1. Cria a rede
G = nx.DiGraph()

# 2. Localiza o arquivo de dados da instancia (toy3 / sintetica12)
pasta_atual = os.path.dirname(os.path.abspath(__file__))
candidatos_txt = [
    os.path.join(pasta_atual, 'toy3.txt'),
    os.path.join(pasta_atual, '..', 'instancias', 'sintetica12.txt'),
    os.path.join(pasta_atual, '..', 'instancias', 'toy3.txt'),
    os.path.join('instancias', 'sintetica12.txt'),
    os.path.join('instancias', 'toy3.txt')
]
caminho_txt = next((c for c in candidatos_txt if os.path.exists(c)), 'sintetica12.txt')

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

# 4. Adiciona as conexoes exatas (1232.24 metros - Topologia Blindada)
conexoes = [
    (1, 2), (9, 3), (10, 9), (11, 10), (26, 11), (11, 12),
    (12, 13), (22, 21), (2, 22), (2, 26), (11, 27), (27, 28),
    (28, 29), (29, 30), (30, 31), (31, 32), (3, 41), (32, 42),
    (32, 43), (27, 44), (29, 45), (12, 46), (13, 47), (21, 48),
    (22, 49), (31, 50), (29, 51), (3, 52)
]
G.add_edges_from(conexoes)

# 5. Desenha a rede usando as posicoes geograficas do arquivo
plt.figure(figsize=(12, 8))

# Separa os nós em três listas distintas
lista_olt = [1]
lista_clientes = [n for n in G.nodes() if n >= 41]
lista_postes = [n for n in G.nodes() if n != 1 and n < 41]

# Desenha cada grupo com formas e tons de cinza diferentes
# 's' = square (quadrado), '^' = triangle (triângulo), 'o' = circle (círculo)
nx.draw_networkx_nodes(G, pos=posicoes_reais, nodelist=lista_olt, node_color='dimgray', node_shape='s', node_size=600)
nx.draw_networkx_nodes(G, pos=posicoes_reais, nodelist=lista_clientes, node_color='darkgray', node_shape='^', node_size=600)
nx.draw_networkx_nodes(G, pos=posicoes_reais, nodelist=lista_postes, node_color='lightgray', node_shape='o', node_size=600)

# Desenha as setas (arestas)
nx.draw_networkx_edges(G, pos=posicoes_reais, arrows=True)

# Coloca a OLT com número branco e o restante com número preto para dar contraste
nx.draw_networkx_labels(G, pos=posicoes_reais, labels={1: 1}, font_color='white', font_weight='bold')
outros_nos = {n: n for n in G.nodes() if n != 1}
nx.draw_networkx_labels(G, pos=posicoes_reais, labels=outros_nos, font_color='black', font_weight='bold')

plt.title("Rede FTTx (Topologia Blindada) - Custo: 1232.24m")
plt.grid(True, linestyle='--', alpha=0.6)

# Salva a imagem em alta resolução (300 dpi) na pasta imagens
caminho_img = os.path.join(pasta_atual, 'imagens', 'grafo_otimizado.png')
os.makedirs(os.path.dirname(caminho_img), exist_ok=True)
plt.savefig(caminho_img, format='png', dpi=300, bbox_inches='tight')
plt.show()