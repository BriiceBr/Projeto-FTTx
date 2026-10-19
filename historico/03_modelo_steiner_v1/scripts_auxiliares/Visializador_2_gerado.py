import networkx as nx
import matplotlib.pyplot as plt
import os

G_total = nx.Graph() 
G_rota = nx.DiGraph()

pasta_atual = os.path.dirname(os.path.abspath(__file__))
caminho_txt = os.path.join(pasta_atual, 'cidade_teste.txt')

posicoes_reais = {}
lendo_nos = False
lendo_arcos = False

# Leitura completa da planta da cidade
with open(caminho_txt, 'r') as arq:
    for linha in arq:
        if 'Nodes' in linha:
            lendo_nos = True
            lendo_arcos = False
            continue
        if 'Arcs' in linha:
            lendo_nos = False
            lendo_arcos = True
            continue
        if 'Splitters' in linha:
            lendo_arcos = False

        if lendo_nos:
            partes = linha.split()
            if len(partes) >= 3:
                id_no = int(partes[0])
                x = float(partes[1])
                y = float(partes[2])
                posicoes_reais[id_no] = (x, y)
                G_total.add_node(id_no)
                G_rota.add_node(id_no)

        if lendo_arcos:
            partes = linha.split()
            if len(partes) >= 2:
                G_total.add_edge(int(partes[0]), int(partes[1]))

# Conexoes extraidas da sua imagem
conexoes = [
    (1, 15), (15, 29), (29, 43), (43, 57), (57, 71), (71, 85),
    (102, 88), (88, 89), (89, 90), (90, 91), (91, 92), (92, 93),
    (93, 94), (94, 95), (95, 96), (96, 97), (97, 98), (85, 99),
    (99, 100), (100, 101), (101, 102), (89, 103), (90, 104),
    (91, 105), (92, 106), (93, 107), (94, 108), (95, 109),
    (96, 110), (97, 111), (98, 112), (99, 113), (100, 114), (101, 115)
]
G_rota.add_edges_from(conexoes)

plt.figure(figsize=(15, 10))

cores = []
ID_INICIO_CLIENTES = 102 

for node in G_total.nodes():
    if node == 1:
        cores.append('lightgreen')
    elif node >= ID_INICIO_CLIENTES:
        cores.append('salmon')
    else:
        cores.append('lightblue')

# 1. Desenha a malha da cidade inteira em cinza claro no fundo
nx.draw_networkx_edges(G_total, pos=posicoes_reais, edge_color='lightgray', style='dashed', width=1.0)
nx.draw_networkx_nodes(G_total, pos=posicoes_reais, node_color=cores, node_size=250)
nx.draw_networkx_labels(G_total, pos=posicoes_reais, font_size=7)

# 2. Desenha a rota escolhida pelo algoritmo por cima em preto
nx.draw_networkx_edges(G_rota, pos=posicoes_reais, edge_color='black', edgelist=conexoes, width=2.5, arrows=True, arrowsize=15)

plt.title("Rede FTTx - Cidade Completa vs Caminho Escolhido")
plt.axis('off')
plt.show()