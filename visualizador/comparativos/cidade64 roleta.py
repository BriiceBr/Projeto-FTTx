import matplotlib.pyplot as plt
import networkx as nx
import os

# Dados da execucao
nome_arquivo = 'cidade64.txt'
metros = 4695.70
preco = metros * 1.9
taxa_mutacao = '5%'
geracao_parada = 401

conexoes = [
    (1, 2), (2, 3), (3, 4), (4, 5), (5, 6), (6, 7), (7, 8), (8, 9), (9, 10), (10, 11),
    (11, 12), (12, 13), (13, 14), (14, 15), (15, 16), (16, 17), (17, 18), (39, 20),
    (20, 21), (21, 22), (22, 23), (25, 24), (26, 25), (27, 26), (45, 27), (23, 28),
    (28, 29), (29, 30), (30, 31), (34, 33), (35, 34), (36, 35), (37, 36), (38, 37),
    (20, 38), (40, 39), (41, 40), (42, 41), (59, 42), (42, 43), (43, 44), (44, 45),
    (35, 46), (46, 47), (47, 48), (50, 49), (51, 50), (52, 51), (15, 52), (15, 53),
    (53, 54), (54, 55), (55, 56), (56, 57), (57, 58), (58, 59), (63, 62), (64, 63),
    (65, 64), (66, 65), (10, 66), (56, 73), (75, 74), (76, 75), (77, 76), (78, 77),
    (7, 78), (5, 85), (4, 86), (2, 87), (4, 88), (3, 89), (6, 90), (2, 91), (3, 92),
    (6, 93), (78, 94), (73, 95), (7, 96), (10, 97), (74, 98), (75, 99), (12, 100),
    (15, 101), (13, 102), (13, 103), (13, 104), (14, 105), (62, 106), (51, 107),
    (16, 108), (58, 109), (15, 110), (73, 111), (13, 112), (13, 113), (58, 114),
    (54, 115), (56, 116), (39, 117), (48, 118), (20, 119), (39, 120), (18, 121),
    (39, 122), (17, 123), (49, 124), (47, 125), (34, 126), (34, 127), (46, 128),
    (49, 129), (49, 130), (46, 131), (34, 132), (31, 133), (29, 134), (30, 135),
    (33, 136), (33, 137), (47, 138), (33, 139), (28, 140), (24, 141), (29, 142),
    (23, 143), (24, 144), (24, 145), (29, 146), (24, 147), (24, 148)
]

G = nx.Graph()
pasta_atual = os.path.dirname(os.path.abspath(__file__))
candidatos = [
    os.path.join(pasta_atual, nome_arquivo),
    os.path.join(pasta_atual, '..', '..', 'instancias', nome_arquivo),
    os.path.join('instancias', nome_arquivo)
]
caminho_txt = next((c for c in candidatos if os.path.exists(c)), os.path.join(pasta_atual, nome_arquivo))

posicoes = {}
lista_olt = []
lista_postes = []
lista_clientes = []

with open(caminho_txt, 'r') as arq:
    lendo_nos = False
    num_clientes = 0
    total_nos = 0
    
    for linha in arq:
        if 'Clients' in linha:
            num_clientes = int(linha.split()[1])
        if 'Nodes' in linha:
            total_nos = int(linha.split()[1])
            lendo_nos = True
            continue
        if 'Arc' in linha or 'BalancedSplitters' in linha:
            lendo_nos = False
            
        if lendo_nos:
            partes = linha.split()
            if len(partes) >= 3:
                id_no = int(partes[0])
                y = float(partes[1])
                x = float(partes[2])
                posicoes[id_no] = (x, y)
                G.add_node(id_no)
                
                if id_no == 1:
                    lista_olt.append(id_no)
                elif id_no > (total_nos - num_clientes):
                    lista_clientes.append(id_no)
                else:
                    lista_postes.append(id_no)

if conexoes:
    G.add_edges_from(conexoes)

plt.figure(figsize=(14, 8))

nx.draw_networkx_nodes(G, pos=posicoes, nodelist=lista_olt, node_color='orange', edgecolors='black', node_size=70, label='OLT')
nx.draw_networkx_nodes(G, pos=posicoes, nodelist=lista_postes, node_color='blue', alpha=0.7, node_size=30, label='Poste')
nx.draw_networkx_nodes(G, pos=posicoes, nodelist=lista_clientes, node_color='crimson', node_size=35, label='Cliente')

nx.draw_networkx_edges(G, pos=posicoes, edgelist=conexoes, width=1.2, edge_color='black')

titulo_grafico = f"Instancia: {nome_arquivo} | Metros: {metros:.2f}m | Preco: R$ {preco:.2f}\nMutacao: {taxa_mutacao} | Geracao Parada: {geracao_parada}"
plt.title(titulo_grafico, fontsize=11)

plt.legend(loc='lower right', scatterpoints=1)
plt.axis('off')

plt.tight_layout()
plt.show()