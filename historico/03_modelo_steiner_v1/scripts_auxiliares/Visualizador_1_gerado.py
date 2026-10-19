import networkx as nx
import matplotlib.pyplot as plt
import os

G = nx.DiGraph()

pasta_atual = os.path.dirname(os.path.abspath(__file__))
caminho_txt = os.path.join(pasta_atual, 'cidade_teste.txt')

posicoes_reais = {}
lendo_nos = False

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

# Conexoes extraidas da sua imagem
conexoes = [
    (1, 15), (15, 29), (29, 43), (43, 57), (57, 71), (71, 85),
    (102, 88), (88, 89), (89, 90), (90, 91), (91, 92), (92, 93),
    (93, 94), (94, 95), (95, 96), (96, 97), (97, 98), (85, 99),
    (99, 100), (100, 101), (101, 102), (89, 103), (90, 104),
    (91, 105), (92, 106), (93, 107), (94, 108), (95, 109),
    (96, 110), (97, 111), (98, 112), (99, 113), (100, 114), (101, 115)
]
G.add_edges_from(conexoes)

plt.figure(figsize=(14, 10))

cores = []
ID_INICIO_CLIENTES = 102 

for node in G.nodes():
    if node == 1:
        cores.append('lightgreen')
    elif node >= ID_INICIO_CLIENTES:
        cores.append('salmon')
    else:
        cores.append('lightblue')

nx.draw(G, pos=posicoes_reais, with_labels=True, node_color=cores, node_size=300, font_size=8, arrows=True)
plt.title("Rede FTTx - Rota Limpa (Custo: 680.00m)")
plt.show()