import matplotlib.pyplot as plt
import networkx as nx
import os

# Dados da execucao (Metodo de Torneio)
nome_arquivo = 'interior64.txt'
metros = 8409.25
preco = metros * 1.9
taxa_mutacao = '5%'
geracao_parada = 2000

conexoes = [
    (1, 2), (2, 3), (3, 4), (4, 5), (5, 6), (6, 7), (7, 8), (8, 9), (9, 10), (10, 11),
    (11, 12), (12, 13), (13, 14), (14, 15), (15, 16), (16, 17), (17, 18), (18, 19),
    (19, 20), (20, 21), (21, 22), (22, 23), (23, 24), (24, 25), (25, 26), (26, 27),
    (27, 28), (28, 29), (29, 30), (30, 31), (31, 32), (32, 33), (33, 34), (34, 35),
    (35, 36), (36, 37), (37, 38), (38, 39), (39, 40), (40, 41), (41, 42), (38, 45),
    (45, 46), (38, 49), (49, 50), (50, 51), (51, 52), (52, 53), (53, 54), (54, 55),
    (55, 56), (56, 57), (57, 58), (58, 59), (59, 60), (60, 61), (66, 64), (59, 66),
    (57, 67), (67, 68), (68, 69), (69, 70), (70, 71), (71, 72), (72, 73), (73, 74),
    (74, 75), (75, 76), (76, 77), (77, 78), (78, 79), (79, 80), (78, 82), (82, 83),
    (83, 84), (84, 85), (85, 86), (86, 87), (87, 88), (88, 89), (89, 90), (90, 91),
    (91, 92), (92, 93), (93, 94), (94, 95), (95, 96), (96, 97), (97, 98), (98, 99),
    (99, 100), (100, 101), (101, 102), (102, 103), (103, 104), (104, 105), (28, 112),
    (41, 113), (50, 114), (50, 115), (50, 116), (51, 117), (53, 118), (51, 119),
    (53, 120), (51, 121), (54, 122), (52, 123), (50, 124), (69, 125), (59, 126),
    (37, 127), (59, 128), (59, 129), (70, 130), (61, 131), (55, 132), (54, 133),
    (67, 134), (64, 135), (64, 136), (66, 137), (66, 138), (68, 139), (69, 140),
    (71, 141), (71, 142), (83, 143), (78, 144), (83, 145), (87, 146), (87, 147),
    (88, 148), (93, 149), (80, 150), (80, 151), (75, 152), (95, 153), (94, 154),
    (93, 155), (88, 156), (96, 157), (91, 158), (84, 159), (95, 160), (71, 161),
    (71, 162), (41, 163), (51, 164), (38, 165), (38, 166), (40, 167), (38, 168),
    (46, 169), (49, 170), (92, 171), (94, 172), (99, 173), (102, 174), (105, 175)
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

titulo_grafico = f"Instancia: {nome_arquivo} | Metros: {metros:.2f}m | Preco: R$ {preco:.2f}\nMutacao: {taxa_mutacao} | Geracao Parada: {geracao_parada} (Torneio)"
plt.title(titulo_grafico, fontsize=11)

plt.legend(loc='lower right', scatterpoints=1)
plt.axis('off')

plt.tight_layout()
plt.show()