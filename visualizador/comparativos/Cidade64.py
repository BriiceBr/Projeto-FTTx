import matplotlib.pyplot as plt
import networkx as nx
import os

# 1. Cria o grafo nÃ£o-direcionado (sem setas grossas, apenas linhas finas como na imagem de referÃªncia)
G = nx.Graph()

pasta_atual = os.path.dirname(os.path.abspath(__file__))
candidatos = [
    os.path.join(pasta_atual, 'cidade64.txt'),
    os.path.join(pasta_atual, '..', '..', 'instancias', 'cidade64.txt'),
    os.path.join('instancias', 'cidade64.txt')
]
caminho_txt = next((c for c in candidatos if os.path.exists(c)), 'cidade64.txt')

posicoes = {}
lista_olt = []
lista_postes = []
lista_clientes = []

# 2. LÃª TODOS os nÃ³s do mapa para desenhar o cenÃ¡rio completo
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
                # Em GPS: partes[1] Ã© Latitude (Y), partes[2] Ã© Longitude (X)
                y = float(partes[1])
                x = float(partes[2])
                posicoes[id_no] = (x, y)
                G.add_node(id_no) # Adiciona todos os nÃ³s no grafo
                
                # Classifica pelas cores
                if id_no == 1:
                    lista_olt.append(id_no)
                elif id_no > (total_nos - num_clientes):
                    lista_clientes.append(id_no)
                else:
                    lista_postes.append(id_no)

# 3. Adiciona as conexoes exatas (4246.33 metros) que o seu C gerou
conexoes = [
    (1, 2), (2, 3), (3, 4), (4, 5), (5, 6), (6, 7), (7, 8), (8, 9), (9, 10), (10, 11), 
    (11, 12), (12, 13), (13, 14), (14, 15), (15, 16), (16, 17), (17, 18), (18, 19), 
    (19, 20), (20, 21), (21, 22), (22, 23), (23, 24), (23, 28), (32, 31), (33, 32), 
    (34, 33), (35, 34), (46, 35), (47, 46), (48, 47), (49, 48), (50, 49), (51, 50), 
    (52, 51), (15, 52), (73, 56), (56, 57), (10, 67), (67, 68), (68, 69), (69, 70), 
    (70, 71), (71, 72), (72, 73), (7, 78), (5, 85), (5, 86), (2, 87), (4, 88), (4, 89), 
    (5, 90), (2, 91), (3, 92), (6, 93), (78, 94), (72, 95), (7, 96), (10, 97), (78, 98), 
    (78, 99), (12, 100), (15, 101), (14, 102), (14, 103), (13, 104), (15, 105), 
    (12, 106), (52, 107), (16, 108), (57, 109), (16, 110), (71, 111), (13, 112), 
    (14, 113), (57, 114), (14, 115), (73, 116), (21, 117), (48, 118), (20, 119), 
    (20, 120), (18, 121), (19, 122), (17, 123), (49, 124), (48, 125), (34, 126), 
    (46, 127), (46, 128), (49, 129), (49, 130), (46, 131), (34, 132), (32, 133), 
    (31, 134), (31, 135), (33, 136), (33, 137), (48, 138), (33, 139), (28, 140), 
    (23, 141), (28, 142), (28, 143), (24, 144), (24, 145), (28, 146), (24, 147), (23, 148)
]
G.add_edges_from(conexoes)

# 4. Desenha a rede graficamente
plt.figure(figsize=(14, 8))

# OLT: Laranja com borda preta
nx.draw_networkx_nodes(G, pos=posicoes, nodelist=lista_olt, node_color='orange', edgecolors='black', node_size=60, label='OLT')

# Postes: Azul
nx.draw_networkx_nodes(G, pos=posicoes, nodelist=lista_postes, node_color='blue', alpha=0.8, node_size=35, label='Poste')

# Clientes: Vermelho
nx.draw_networkx_nodes(G, pos=posicoes, nodelist=lista_clientes, node_color='crimson', node_size=35, label='Cliente')

# Arestas (Cabos) em linha preta fina
nx.draw_networkx_edges(G, pos=posicoes, edgelist=conexoes, width=1.0, edge_color='black')

# Legenda e limpeza do fundo
plt.legend(loc='lower right', scatterpoints=1)
plt.axis('off') # Remove eixos e linhas de grade
plt.title("Rede cidade64 - Topologia Otimizada (4246.33m)", fontsize=12)

# Salva imagem e exibe
plt.tight_layout()
plt.savefig(os.path.join(pasta_atual, 'cidade64_verificacao.png'), dpi=300)
plt.show()