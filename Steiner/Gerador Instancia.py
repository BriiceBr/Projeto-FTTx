import os

total_nos = 126
clientes = 25
colunas = 14 # Cria uma grade de 14 colunas por 9 linhas

# Caminho exato da sua pasta
pasta_destino = r"C:\Users\diogo\OneDrive\Área de Trabalho\Projeto-FTTx\Steiner"
caminho_arquivo = os.path.join(pasta_destino, "cidade_teste.txt")

with open(caminho_arquivo, "w") as f:
    f.write(f"Clients {clientes}\n")
    f.write(f"Nodes {total_nos}\n")

    # 1. Gera as coordenadas dos 126 postes (distancia de 20m entre cada)
    for i in range(1, total_nos + 1):
        x = ((i - 1) % colunas) * 20.0
        y = ((i - 1) // colunas) * 20.0
        f.write(f"{i} {x:.1f} {y:.1f}\n")

    # 2. Gera as ruas (ligando postes na horizontal e vertical)
    arcos = []
    for i in range(1, total_nos + 1):
        # Liga com o poste da direita
        if i % colunas != 0 and i + 1 <= total_nos:
            arcos.append((i, i + 1))
            arcos.append((i + 1, i)) # Mao dupla
        # Liga com o poste de baixo
        if i + colunas <= total_nos:
            arcos.append((i, i + colunas))
            arcos.append((i + colunas, i)) # Mao dupla

    f.write(f"Arcs {len(arcos)}\n")
    for origem, destino in arcos:
        f.write(f"{origem} {destino}\n")

print(f"Arquivo 'cidade_teste.txt' gerado com sucesso na pasta: \n{pasta_destino}")