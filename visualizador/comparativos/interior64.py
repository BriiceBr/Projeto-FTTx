import matplotlib.pyplot as plt
import networkx as nx
import os

# 1. Configuração da Instância e Conexões geradas pelo seu algoritmo (9394.21 metros)
NOME_ARQUIVO = 'interior64.txt'
CONEXOES = [
    (1, 2), (2, 3), (3, 4), (4, 5), (5, 6), (6, 7), (7, 8), (8, 9), (9, 10), (10, 11),
    (11, 12), (12, 13), (13, 14), (14, 15), (15, 16), (16, 17), (17, 18), (18, 19), 
    (19, 20), (20, 21), (21, 22), (22, 23), (23, 24), (24, 25), (25, 26), (26, 27), 
    (27, 28), (28, 29), (29, 30), (30, 31), (31, 32), (32, 33), (33, 34), (34, 35), 
    (35, 36), (36, 37), (37, 38), (38, 45), (45, 46), (46, 47), (38, 49), (49, 50), 
    (50, 51), (51, 52), (52, 53), (53, 54), (54, 55), (55, 56), (56, 57), (57, 58), 
    (58, 59), (59, 60), (60, 61), (61, 62), (62, 63), (66, 64), (59, 66), (57, 67), 
    (67, 68), (68, 69), (69, 70), (70, 71), (71, 72), (72, 73), (73, 74), (74, 75), 
    (75, 76), (76, 77), (77, 78), (78, 79), (79, 80), (80, 81), (78, 82), (82, 83), 
    (83, 84), (84, 85), (85, 86), (86, 87), (87, 88), (88, 89), (89, 90), (90, 91), 
    (91, 92), (92, 93), (93, 94), (94, 95), (95, 96), (96, 97), (97, 98), (98, 99), 
    (99, 100), (100, 101), (101, 102), (102, 103), (28, 112), (32, 113), (51, 114), 
    (28, 115), (32, 116), (34, 117), (53, 118), (53, 119), (53, 120), (52, 121), 
    (54, 122), (53, 123), (51, 124), (70, 125), (58, 126), (28, 127), (68, 128), 
    (59, 129), (69, 130), (63, 131), (53, 132), (53, 133), (67, 134), (64, 135), 
    (64, 136), (66, 137), (59, 138), (57, 139), (70, 140), (71, 141), (71, 142), 
    (82, 143), (83, 144), (85, 145), (87, 146), (87, 147), (89, 148), (94, 149), 
    (81, 150), (80, 151), (82, 152), (96, 153), (95, 154), (93, 155), (88, 156), 
    (96, 157), (92, 158), (87, 159), (96, 160), (72, 161), (70, 162), (34, 163), 
    (35, 164), (34, 165), (32, 166), (37, 167), (51, 168), (47, 169), (28, 170), 
    (92, 171), (96, 172), (100, 173), (101, 174), (103, 175)
]

# 2. Leitura de todos os pontos do mapa
G = nx.Graph()
candidatos = [
    os.path.join(pasta_atual, NOME_ARQUIVO),
    os.path.join(pasta_atual, '..', '..', 'instancias', NOME_ARQUIVO),
    os.path.join('instancias', NOME_ARQUIVO)
]
caminho_txt = next((c for c in candidatos if os.path.exists(c)), NOME_ARQUIVO)

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
                # Mapeamento GPS
                y = float(partes[1])
                x = float(partes[2])
                posicoes[id_no] = (x, y)
                G.add_node(id_no)
                
                # Classificação para as cores
                if id_no == 1:
                    lista_olt.append(id_no)
                elif id_no > (total_nos - num_clientes):
                    lista_clientes.append(id_no)
                else:
                    lista_postes.append(id_no)

# 3. Insere as rotas calculadas pelo Algoritmo Genético
if CONEXOES:
    G.add_edges_from(CONEXOES)

# 4. Desenho do Gráfico no padrão da Dissertação
plt.figure(figsize=(14, 8))

# Nós sem texto (bolinhas puras)
nx.draw_networkx_nodes(G, pos=posicoes, nodelist=lista_olt, node_color='orange', edgecolors='black', node_size=70, label='OLT')
nx.draw_networkx_nodes(G, pos=posicoes, nodelist=lista_postes, node_color='blue', alpha=0.7, node_size=30, label='Poste')
nx.draw_networkx_nodes(G, pos=posicoes, nodelist=lista_clientes, node_color='crimson', node_size=35, label='Cliente')

# Arestas do cabeamento
nx.draw_networkx_edges(G, pos=posicoes, edgelist=CONEXOES, width=1.2, edge_color='black')

plt.legend(loc='lower right', scatterpoints=1)
plt.axis('off')
plt.title(f"Rede interior64 - Topologia Otimizada (9394.21m)", fontsize=14)

plt.tight_layout()
plt.show()