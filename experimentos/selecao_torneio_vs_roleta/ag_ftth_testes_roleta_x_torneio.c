#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

#define RAIO_TERRA 6371000.0
#define PI 3.14159265358979323846

#define TAM_MAPA 175
#define TAM_P 100

#define TAX_MUTACAO_POP_PADRAO 20
#define TAX_MUTACAO_GENE_PADRAO 2
#define LIMITE_ESTAGNACAO 100000
#define MAX_TENTATIVAS_DUPLICATA 5

#define TOP_K_INICIAL 5
#define TOP_K_CRUZAMENTO 5
#define TOP_K_REPARO 5
#define MAX_TOP_K_SELECAO 10
#define TAM_BLOCO 3
#define MAX_CANDIDATOS (TAM_MAPA * TAM_MAPA)

struct poste {
    int id;
    double x;
    double y;
    int tipo; /* 0 = OLT, 1 = Poste de Passagem, 2 = Cliente */
};

struct individuo {
    int pai[TAM_MAPA];
    double distancia_total;
    double atenuacao_maxima;
};

struct candidato {
    int origem;
    int destino;
    int categoria;
    int fonte;
    double distancia;
};

static struct candidato candidatos_trabalho[MAX_CANDIDATOS];

struct poste mapa[TAM_MAPA];
int matriz_arcos[TAM_MAPA][TAM_MAPA];

double matriz_distancias[TAM_MAPA][TAM_MAPA];
int vizinhos[TAM_MAPA][TAM_MAPA];
int grau_vizinho[TAM_MAPA];

struct individuo pop[TAM_P];
struct individuo pop_nova[TAM_P];

int tipo_calculo;
int total_clientes = 0;

/* ------------------------------------------------------------ */
/* Utilitarios                                                   */
/* ------------------------------------------------------------ */

double para_radianos(double graus) {
    return graus * PI / 180.0;
}

double calculo_pitagoras(struct poste p1, struct poste p2) {
    double dx = p1.x - p2.x;
    double dy = p1.y - p2.y;
    return sqrt((dx * dx) + (dy * dy));
}

double calculo_coordenadas_reais(struct poste p1, struct poste p2) {
    double lat1 = para_radianos(p1.x);
    double lon1 = para_radianos(p1.y);
    double lat2 = para_radianos(p2.x);
    double lon2 = para_radianos(p2.y);

    double dlat = lat2 - lat1;
    double dlon = lon2 - lon1;

    double a = sin(dlat / 2.0) * sin(dlat / 2.0) +
               cos(lat1) * cos(lat2) *
               sin(dlon / 2.0) * sin(dlon / 2.0);

    if (a < 0.0) a = 0.0;
    if (a > 1.0) a = 1.0;

    double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
    return RAIO_TERRA * c;
}

double calcular_distancia(struct poste p1, struct poste p2) {
    if (tipo_calculo == 1) {
        return calculo_pitagoras(p1, p2);
    }
    return calculo_coordenadas_reais(p1, p2);
}

int tem_arco(int u, int v) {
    return matriz_arcos[u][v] || matriz_arcos[v][u];
}

void preparar_grafo(void) {
    for (int i = 0; i < TAM_MAPA; i++) {
        grau_vizinho[i] = 0;

        for (int j = 0; j < TAM_MAPA; j++) {
            if (i == j) {
                matriz_distancias[i][j] = 0.0;
                continue;
            }

            matriz_distancias[i][j] = calcular_distancia(mapa[i], mapa[j]);

            if (tem_arco(i, j)) {
                vizinhos[i][grau_vizinho[i]++] = j;
            }
        }
    }
}

double distancia_aresta(int u, int v) {
    return matriz_distancias[u][v];
}

int possui_aresta_pai(const struct individuo *ind, int u, int v) {
    return (ind->pai[u] == v || ind->pai[v] == u);
}

int fonte_aresta(const struct individuo *paiA, const struct individuo *paiB, int u, int v) {
    int emA = possui_aresta_pai(paiA, u, v);
    int emB = possui_aresta_pai(paiB, u, v);

    if (emA && emB) return 3;
    if (emA) return 1;
    if (emB) return 2;
    return 0;
}

/* ------------------------------------------------------------ */
/* Leitura da instancia                                         */
/* ------------------------------------------------------------ */

void ler_instancia_fttx(const char nome_arq[]) {
    FILE *arq = fopen(nome_arq, "r");
    if (arq == NULL) {
        char caminho_alt[512];
        snprintf(caminho_alt, sizeof(caminho_alt), "instancias/%s", nome_arq);
        arq = fopen(caminho_alt, "r");
        if (arq == NULL) {
            snprintf(caminho_alt, sizeof(caminho_alt), "../instancias/%s", nome_arq);
            arq = fopen(caminho_alt, "r");
            if (arq == NULL) {
                snprintf(caminho_alt, sizeof(caminho_alt), "../../instancias/%s", nome_arq);
                arq = fopen(caminho_alt, "r");
            }
        }
    }
    if (arq == NULL) {
        printf("Erro ao abrir arquivo %s!\n", nome_arq);
        printf("Certifique-se de que o arquivo esta na pasta atual ou em 'instancias/'.\n");
        exit(1);
    }

    char linha[256];
    int lendo_nos = 0;
    int lendo_arcos = 0;
    total_clientes = 0;

    memset(mapa, 0, sizeof(mapa));
    memset(matriz_arcos, 0, sizeof(matriz_arcos));
    memset(matriz_distancias, 0, sizeof(matriz_distancias));
    memset(vizinhos, 0, sizeof(vizinhos));
    memset(grau_vizinho, 0, sizeof(grau_vizinho));

    while (fgets(linha, sizeof(linha), arq)) {
        if (strstr(linha, "Clients") != NULL) {
            sscanf(linha, "Clients %d", &total_clientes);
            continue;
        }

        if (strstr(linha, "Nodes") != NULL) {
            lendo_nos = 1;
            lendo_arcos = 0;
            continue;
        }

        if (strstr(linha, "Arc") != NULL || strstr(linha, "Arcs") != NULL) {
            lendo_nos = 0;
            lendo_arcos = 1;
            continue;
        }

        if (strstr(linha, "Splitters") != NULL ||
            strstr(linha, "BalancedSplitters") != NULL ||
            strstr(linha, "UnbalancedSplitters") != NULL) {
            lendo_arcos = 0;
        }

        if (lendo_nos) {
            int id_lido;
            double x, y;

            if (sscanf(linha, "%d %lf %lf", &id_lido, &x, &y) == 3) {
                int id = id_lido - 1;

                if (id >= 0 && id < TAM_MAPA) {
                    mapa[id].id = id;
                    mapa[id].x = x;
                    mapa[id].y = y;

                    if (id_lido == 1) {
                        mapa[id].tipo = 0;
                    } else if (total_clientes > 0 && id_lido > (TAM_MAPA - total_clientes)) {
                        mapa[id].tipo = 2;
                    } else {
                        mapa[id].tipo = 1;
                    }
                }
            }
        }

        if (lendo_arcos) {
            int origem, destino;
            if (sscanf(linha, "%d %d", &origem, &destino) == 2) {
                origem--;
                destino--;

                if (origem >= 0 && origem < TAM_MAPA && destino >= 0 && destino < TAM_MAPA) {
                    matriz_arcos[origem][destino] = 1;
                }
            }
        }
    }

    fclose(arq);

    if (total_clientes <= 0 || total_clientes > TAM_MAPA - 1) {
        printf("Erro: quantidade de clientes invalida (%d).\n", total_clientes);
        exit(1);
    }
}

/* ------------------------------------------------------------ */
/* Validacao / estado da arvore                                 */
/* ------------------------------------------------------------ */

int todos_clientes_conectados(const int visitado[]) {
    for (int i = 0; i < TAM_MAPA; i++) {
        if (mapa[i].tipo == 2 && !visitado[i]) {
            return 0;
        }
    }
    return 1;
}

int forma_ciclo(int candidato_pai, int no_mutado, const int vetor_pais[]) {
    int atual = candidato_pai;
    int guard = 0;

    while (atual != -1 && guard < TAM_MAPA) {
        if (atual == no_mutado) return 1;
        atual = vetor_pais[atual];
        guard++;
    }
    if (guard >= TAM_MAPA) return 1;
    return 0;
}

int no_ativo(const struct individuo *ind, int no) {
    return (no == 0 || ind->pai[no] != -1);
}

/* ------------------------------------------------------------ */
/* Inicializacao: Prim guloso-probabilistico                     */
/* ------------------------------------------------------------ */

int escolher_candidato_ponderado(struct candidato candidatos[], int n, int k) {
    int melhores[MAX_TOP_K_SELECAO];
    double melhores_dist[MAX_TOP_K_SELECAO];
    int quantidade = 0;

    if (n <= 0) return -1;
    if (k < 1) k = 1;
    if (k > MAX_TOP_K_SELECAO) k = MAX_TOP_K_SELECAO;
    if (k > n) k = n;

    for (int i = 0; i < n; i++) {
        double d = candidatos[i].distancia;
        if (quantidade < k) {
            int pos = quantidade;
            melhores[pos] = i;
            melhores_dist[pos] = d;
            quantidade++;

            while (pos > 0 && melhores_dist[pos] < melhores_dist[pos - 1]) {
                int ti = melhores[pos]; melhores[pos] = melhores[pos - 1]; melhores[pos - 1] = ti;
                double td = melhores_dist[pos]; melhores_dist[pos] = melhores_dist[pos - 1]; melhores_dist[pos - 1] = td;
                pos--;
            }
        } else if (d < melhores_dist[quantidade - 1]) {
            int pos = quantidade - 1;
            melhores[pos] = i;
            melhores_dist[pos] = d;

            while (pos > 0 && melhores_dist[pos] < melhores_dist[pos - 1]) {
                int ti = melhores[pos]; melhores[pos] = melhores[pos - 1]; melhores[pos - 1] = ti;
                double td = melhores_dist[pos]; melhores_dist[pos] = melhores_dist[pos - 1]; melhores_dist[pos - 1] = td;
                pos--;
            }
        }
    }

    int soma_pesos = quantidade * (quantidade + 1) / 2;
    int sorteio = 1 + rand() % soma_pesos;
    int acumulado = 0;

    for (int rank = 0; rank < quantidade; rank++) {
        int peso = quantidade - rank;
        acumulado += peso;
        if (sorteio <= acumulado) return melhores[rank];
    }
    return melhores[quantidade - 1];
}

void gerar_primeira_pop(struct individuo *ind, int indice_populacao) {
    struct candidato *candidatos = candidatos_trabalho;
    int visitado[TAM_MAPA];

    for (int i = 0; i < TAM_MAPA; i++) {
        ind->pai[i] = -1;
        visitado[i] = 0;
    }

    visitado[0] = 1;
    ind->pai[0] = -1;

    int k_inicial = 4 + (indice_populacao % 5);

    while (!todos_clientes_conectados(visitado)) {
        int num_candidatos = 0;

        for (int i = 0; i < TAM_MAPA; i++) {
            if (!visitado[i] || mapa[i].tipo == 2) continue;

            for (int pos = 0; pos < grau_vizinho[i]; pos++) {
                int j = vizinhos[i][pos];
                if (visitado[j]) continue;

                candidatos[num_candidatos].origem = i;
                candidatos[num_candidatos].destino = j;
                candidatos[num_candidatos].categoria = 0;
                candidatos[num_candidatos].fonte = 0;
                candidatos[num_candidatos].distancia = distancia_aresta(i, j);
                num_candidatos++;
            }
        }

        if (num_candidatos == 0) break;
        int escolhido = escolher_candidato_ponderado(candidatos, num_candidatos, k_inicial);
        if (escolhido < 0) break;

        int origem = candidatos[escolhido].origem;
        int destino = candidatos[escolhido].destino;

        ind->pai[destino] = origem;
        visitado[destino] = 1;
    }
}

/* ------------------------------------------------------------ */
/* Poda e avaliacao                                             */
/* ------------------------------------------------------------ */

void podar_arvore(struct individuo *ind) {
    int filhos[TAM_MAPA];
    int removeu;

    do {
        removeu = 0;
        for (int i = 0; i < TAM_MAPA; i++) filhos[i] = 0;
        for (int i = 0; i < TAM_MAPA; i++) {
            if (ind->pai[i] != -1) filhos[ind->pai[i]]++;
        }
        for (int i = 1; i < TAM_MAPA; i++) {
            if (mapa[i].tipo == 1 && ind->pai[i] != -1 && filhos[i] == 0) {
                ind->pai[i] = -1;
                removeu = 1;
            }
        }
    } while (removeu);
}

int individuo_valido(const struct individuo *ind) {
    for (int i = 1; i < TAM_MAPA; i++) {
        if (ind->pai[i] == -1) continue;
        if (ind->pai[i] == i) return 0;
        if (!matriz_arcos[ind->pai[i]][i] && !matriz_arcos[i][ind->pai[i]]) return 0;
        if (mapa[ind->pai[i]].tipo == 2) return 0;
    }

    for (int i = 1; i < TAM_MAPA; i++) {
        if (ind->pai[i] == -1) continue;
        int atual = i;
        int passos = 0;
        while (atual != 0) {
            if (atual < 0 || atual >= TAM_MAPA || ind->pai[atual] == -1) return 0;
            atual = ind->pai[atual];
            passos++;
            if (passos > TAM_MAPA) return 0;
        }
    }

    for (int i = 0; i < TAM_MAPA; i++) {
        if (mapa[i].tipo == 2) {
            int atual = i;
            int passos = 0;
            while (atual != 0) {
                if (ind->pai[atual] == -1) return 0;
                atual = ind->pai[atual];
                passos++;
                if (passos > TAM_MAPA) return 0;
            }
        }
    }
    return 1;
}

void avaliar_individuo(struct individuo *ind) {
    podar_arvore(ind);
    double soma_metros = 0.0;
    for (int i = 0; i < TAM_MAPA; i++) {
        if (ind->pai[i] != -1) {
            soma_metros += distancia_aresta(i, ind->pai[i]);
        }
    }
    ind->distancia_total = soma_metros;
    ind->atenuacao_maxima = 0.0;
}

/* ------------------------------------------------------------ */
/* Selecao                                                      */
/* ------------------------------------------------------------ */

int torneio_indice(struct individuo pop_atual[]) {
    int s1 = rand() % TAM_P;
    int s2 = rand() % TAM_P;
    int s3 = rand() % TAM_P;
    int vencedor = s1;

    if (pop_atual[s2].distancia_total < pop_atual[vencedor].distancia_total) vencedor = s2;
    if (pop_atual[s3].distancia_total < pop_atual[vencedor].distancia_total) vencedor = s3;
    return vencedor;
}

int roleta_indice(struct individuo pop_atual[]) {
    double soma_avaliacoes = 0.0;
    double fatias[TAM_P];

    for (int i = 0; i < TAM_P; i++) {
        if (pop_atual[i].distancia_total <= 0.0) fatias[i] = 0.0;
        else fatias[i] = 1.0 / pop_atual[i].distancia_total;
        soma_avaliacoes += fatias[i];
    }

    if (soma_avaliacoes <= 0.0) return rand() % TAM_P;

    double giro = ((double)rand() / (double)RAND_MAX) * soma_avaliacoes;
    double acumulador = 0.0;

    for (int i = 0; i < TAM_P; i++) {
        acumulador += fatias[i];
        if (acumulador >= giro) return i;
    }
    return TAM_P - 1;
}

/* ------------------------------------------------------------ */
/* Crossover e Mutacao                                          */
/* ------------------------------------------------------------ */

int copiar_bloco_subarvore(const struct individuo *pai, struct individuo *filho, int raiz, int visitado[], int max_novos) {
    if (max_novos <= 0 || mapa[raiz].tipo == 2) return 0;

    int fila[TAM_MAPA];
    int inicio = 0, fim = 0, adicionados = 0;
    fila[fim++] = raiz;

    while (inicio < fim && adicionados < max_novos) {
        int atual = fila[inicio++];
        if (mapa[atual].tipo == 2) continue;

        for (int filho_no = 1; filho_no < TAM_MAPA; filho_no++) {
            if (pai->pai[filho_no] != atual) continue;
            if (visitado[filho_no]) continue;
            if (!matriz_arcos[atual][filho_no] && !matriz_arcos[filho_no][atual]) continue;

            filho->pai[filho_no] = atual;
            visitado[filho_no] = 1;
            adicionados++;

            if (adicionados >= max_novos) break;
            if (mapa[filho_no].tipo != 2) fila[fim++] = filho_no;
            if (fim >= TAM_MAPA) break;
        }
    }
    return adicionados;
}

int escolher_categoria_top_k(struct candidato candidatos[], int n, int categoria) {
    int melhores[MAX_TOP_K_SELECAO];
    double melhores_dist[MAX_TOP_K_SELECAO];
    int quantidade = 0;

    for (int i = 0; i < n; i++) {
        if (candidatos[i].categoria != categoria) continue;
        double d = candidatos[i].distancia;

        if (quantidade < TOP_K_CRUZAMENTO) {
            int p = quantidade;
            melhores[p] = i; melhores_dist[p] = d;
            quantidade++;
            while (p > 0 && melhores_dist[p] < melhores_dist[p - 1]) {
                int ti = melhores[p]; melhores[p] = melhores[p - 1]; melhores[p - 1] = ti;
                double td = melhores_dist[p]; melhores_dist[p] = melhores_dist[p - 1]; melhores_dist[p - 1] = td;
                p--;
            }
        } else if (d < melhores_dist[quantidade - 1]) {
            int p = quantidade - 1;
            melhores[p] = i; melhores_dist[p] = d;
            while (p > 0 && melhores_dist[p] < melhores_dist[p - 1]) {
                int ti = melhores[p]; melhores[p] = melhores[p - 1]; melhores[p - 1] = ti;
                double td = melhores_dist[p]; melhores_dist[p] = melhores_dist[p - 1]; melhores_dist[p - 1] = td;
                p--;
            }
        }
    }

    if (quantidade == 0) return -1;
    int soma_pesos = quantidade * (quantidade + 1) / 2;
    int sorteio = 1 + rand() % soma_pesos;
    int acumulado = 0;

    for (int rank = 0; rank < quantidade; rank++) {
        acumulado += quantidade - rank;
        if (sorteio <= acumulado) return melhores[rank];
    }
    return melhores[quantidade - 1];
}

struct individuo cruzamento_arvore(const struct individuo *paiA, const struct individuo *paiB) {
    struct individuo filho;
    struct candidato *candidatos = candidatos_trabalho;
    int visitado[TAM_MAPA];

    for (int i = 0; i < TAM_MAPA; i++) {
        filho.pai[i] = -1;
        visitado[i] = 0;
    }

    filho.pai[0] = -1;
    visitado[0] = 1;

    while (!todos_clientes_conectados(visitado)) {
        int num_candidatos = 0;
        for (int i = 0; i < TAM_MAPA; i++) {
            if (!visitado[i] || mapa[i].tipo == 2) continue;

            for (int pos = 0; pos < grau_vizinho[i]; pos++) {
                int j = vizinhos[i][pos];
                if (visitado[j]) continue;

                int fonte = fonte_aresta(paiA, paiB, i, j);
                candidatos[num_candidatos].origem = i;
                candidatos[num_candidatos].destino = j;
                candidatos[num_candidatos].fonte = fonte;
                candidatos[num_candidatos].categoria = (fonte == 3) ? 2 : (fonte != 0 ? 1 : 0);
                candidatos[num_candidatos].distancia = distancia_aresta(i, j);
                num_candidatos++;
            }
        }

        if (num_candidatos == 0) break;

        int tem_comum = 0, tem_herdada = 0;
        for (int i = 0; i < num_candidatos; i++) {
            if (candidatos[i].categoria == 2) tem_comum = 1;
            if (candidatos[i].categoria == 1) tem_herdada = 1;
        }

        int indice = -1;
        int categoria_escolhida;
        int sorteio = rand() % 100;

        if (tem_comum && sorteio < 70) {
            categoria_escolhida = 2;
            indice = escolher_categoria_top_k(candidatos, num_candidatos, categoria_escolhida);
        }
        if (indice == -1 && tem_herdada && sorteio >= 70 && sorteio < 95) {
            categoria_escolhida = 1;
            indice = escolher_categoria_top_k(candidatos, num_candidatos, categoria_escolhida);
        }
        if (indice == -1 && sorteio >= 95) {
            categoria_escolhida = 0;
            indice = escolher_categoria_top_k(candidatos, num_candidatos, categoria_escolhida);
        }
        if (indice == -1 && tem_comum) indice = escolher_categoria_top_k(candidatos, num_candidatos, 2);
        if (indice == -1 && tem_herdada) indice = escolher_categoria_top_k(candidatos, num_candidatos, 1);
        if (indice == -1) indice = escolher_candidato_ponderado(candidatos, num_candidatos, TOP_K_REPARO);

        int origem = candidatos[indice].origem;
        int destino = candidatos[indice].destino;
        int fonte = candidatos[indice].fonte;

        filho.pai[destino] = origem;
        visitado[destino] = 1;

        if (fonte != 0 && mapa[destino].tipo != 2) {
            const struct individuo *fonte_pai = NULL;
            if (fonte == 1) fonte_pai = paiA;
            else if (fonte == 2) fonte_pai = paiB;
            else fonte_pai = (rand() % 2 == 0) ? paiA : paiB;

            int tamanho_bloco = 1 + rand() % TAM_BLOCO;
            copiar_bloco_subarvore(fonte_pai, &filho, destino, visitado, tamanho_bloco);
        }
    }
    return filho;
}

void mutacao_arvore(struct individuo *ind, int taxa_mutacao_pop, int taxa_mutacao_gene) {
    if (taxa_mutacao_pop <= 0 || taxa_mutacao_gene <= 0) return;
    if (taxa_mutacao_pop > 100) taxa_mutacao_pop = 100;
    if (taxa_mutacao_gene > 100) taxa_mutacao_gene = 100;

    if ((rand() % 100) >= taxa_mutacao_pop) return;

    for (int no_mutado = 1; no_mutado < TAM_MAPA; no_mutado++) {
        if (ind->pai[no_mutado] == -1) continue;
        if ((rand() % 100) >= taxa_mutacao_gene) continue;

        int vizinhos_validos[TAM_MAPA];
        double distancias_validas[TAM_MAPA];
        int quantidade = 0;

        for (int pos = 0; pos < grau_vizinho[no_mutado]; pos++) {
            int candidato = vizinhos[no_mutado][pos];
            if (!no_ativo(ind, candidato)) continue;
            if (candidato == no_mutado) continue;
            if (mapa[candidato].tipo == 2) continue;
            if (ind->pai[no_mutado] == candidato) continue;
            if (forma_ciclo(candidato, no_mutado, ind->pai)) continue;

            vizinhos_validos[quantidade] = candidato;
            distancias_validas[quantidade] = distancia_aresta(candidato, no_mutado);
            quantidade++;
        }

        if (quantidade == 0) continue;

        int limite = quantidade;
        if (limite > TOP_K_REPARO) limite = TOP_K_REPARO;

        int melhores[TOP_K_REPARO];
        double melhores_dist[TOP_K_REPARO];
        int qtd_melhores = 0;

        for (int i = 0; i < quantidade; i++) {
            if (qtd_melhores < limite) {
                int p = qtd_melhores;
                melhores[p] = vizinhos_validos[i]; melhores_dist[p] = distancias_validas[i];
                qtd_melhores++;
                while (p > 0 && melhores_dist[p] < melhores_dist[p - 1]) {
                    int ti = melhores[p]; melhores[p] = melhores[p - 1]; melhores[p - 1] = ti;
                    double td = melhores_dist[p]; melhores_dist[p] = melhores_dist[p - 1]; melhores_dist[p - 1] = td;
                    p--;
                }
            } else if (distancias_validas[i] < melhores_dist[qtd_melhores - 1]) {
                int p = qtd_melhores - 1;
                melhores[p] = vizinhos_validos[i]; melhores_dist[p] = distancias_validas[i];
                while (p > 0 && melhores_dist[p] < melhores_dist[p - 1]) {
                    int ti = melhores[p]; melhores[p] = melhores[p - 1]; melhores[p - 1] = ti;
                    double td = melhores_dist[p]; melhores_dist[p] = melhores_dist[p - 1]; melhores_dist[p - 1] = td;
                    p--;
                }
            }
        }

        int soma_pesos = qtd_melhores * (qtd_melhores + 1) / 2;
        int sorteio = 1 + rand() % soma_pesos;
        int acumulado = 0;

        for (int rank = 0; rank < qtd_melhores; rank++) {
            int peso = qtd_melhores - rank;
            acumulado += peso;
            if (sorteio <= acumulado) {
                ind->pai[no_mutado] = melhores[rank];
                break;
            }
        }
    }
}

unsigned long long hash_individuo(const struct individuo *ind) {
    unsigned long long hash = 1469598103934665603ULL;
    for (int i = 0; i < TAM_MAPA; i++) {
        unsigned long long valor = (unsigned long long)(ind->pai[i] + 2);
        hash ^= valor + (unsigned long long)(i + 1) * 1099511628211ULL;
        hash *= 1099511628211ULL;
    }
    return hash;
}

int individuo_igual(const struct individuo *a, const struct individuo *b) {
    for (int i = 0; i < TAM_MAPA; i++) {
        if (a->pai[i] != b->pai[i]) return 0;
    }
    return 1;
}

int individuo_ja_existe(const struct individuo pop_atual[], const unsigned long long hashes[], int quantidade, const struct individuo *ind, unsigned long long hash) {
    for (int i = 0; i < quantidade; i++) {
        if (hashes[i] == hash && individuo_igual(&pop_atual[i], ind)) return 1;
    }
    return 0;
}

int indice_melhor(struct individuo pop_atual[]) {
    int melhor = 0;
    for (int i = 1; i < TAM_P; i++) {
        if (pop_atual[i].distancia_total < pop_atual[melhor].distancia_total) melhor = i;
    }
    return melhor;
}

/* ------------------------------------------------------------ */
/* Funcao de Execucao Isolada do Algoritmo                      */
/* ------------------------------------------------------------ */

void executar_ag(int metodo_selecao, int total_geracoes, int taxa_mutacao_pop, int taxa_mutacao_gene, unsigned int semente, struct individuo *melhor_global_saida, double *tempo_execucao_saida) {

    // Congelar semente para garantir condicoes iniciais exatas
    srand(semente);

    clock_t inicio_tempo = clock();
    unsigned long long hashes_pop[TAM_P];

    for (int i = 0; i < TAM_P; i++) {
        int gerado = 0;
        for (int tentativa = 0; tentativa < MAX_TENTATIVAS_DUPLICATA; tentativa++) {
            gerar_primeira_pop(&pop[i], i + tentativa);
            avaliar_individuo(&pop[i]);

            if (!individuo_valido(&pop[i])) continue;
            unsigned long long hash = hash_individuo(&pop[i]);

            if (!individuo_ja_existe(pop, hashes_pop, i, &pop[i], hash)) {
                hashes_pop[i] = hash;
                gerado = 1;
                break;
            }
        }
        if (!gerado) {
            printf("ERRO: nao foi possivel gerar individuo inicial valido/diverso (%d).\n", i);
            exit(1);
        }
    }

    int melhor_inicial = indice_melhor(pop);
    struct individuo melhor_global = pop[melhor_inicial];
    int geracoes_sem_melhora = 0;

    for (int geracao = 1; geracao <= total_geracoes; geracao++) {
        unsigned long long hashes_nova[TAM_P];
        hashes_nova[0] = hash_individuo(&melhor_global);
        pop_nova[0] = melhor_global;

        for (int i = 1; i < TAM_P; i++) {
            struct individuo filho = melhor_global;
            int filho_aceito = 0;

            for (int tentativa = 0; tentativa < MAX_TENTATIVAS_DUPLICATA; tentativa++) {
                int indice_paiA, indice_paiB;

                if (metodo_selecao == 1) {
                    indice_paiA = torneio_indice(pop);
                    indice_paiB = torneio_indice(pop);
                } else {
                    indice_paiA = roleta_indice(pop);
                    indice_paiB = roleta_indice(pop);
                }

                for (int tentativa_pai = 0; tentativa_pai < 3 && indice_paiB == indice_paiA; tentativa_pai++) {
                    if (metodo_selecao == 1) indice_paiB = torneio_indice(pop);
                    else indice_paiB = roleta_indice(pop);
                }

                filho = cruzamento_arvore(&pop[indice_paiA], &pop[indice_paiB]);
                mutacao_arvore(&filho, taxa_mutacao_pop, taxa_mutacao_gene);
                avaliar_individuo(&filho);

                if (!individuo_valido(&filho)) continue;

                unsigned long long hash = hash_individuo(&filho);
                if (!individuo_ja_existe(pop_nova, hashes_nova, i, &filho, hash)) {
                    hashes_nova[i] = hash;
                    pop_nova[i] = filho;
                    filho_aceito = 1;
                    break;
                }
            }

            if (!filho_aceito) {
                pop_nova[i] = filho;
                hashes_nova[i] = hash_individuo(&pop_nova[i]);
            }
        }

        int melhor_geracao = indice_melhor(pop_nova);

        if (pop_nova[melhor_geracao].distancia_total < melhor_global.distancia_total) {
            melhor_global = pop_nova[melhor_geracao];
            geracoes_sem_melhora = 0;
        } else {
            geracoes_sem_melhora++;
        }

        pop_nova[0] = melhor_global;

        for (int i = 0; i < TAM_P; i++) {
            pop[i] = pop_nova[i];
        }

        if (geracao % 100 == 0 || geracao == total_geracoes) {
            double soma = 0.0;
            for (int i = 0; i < TAM_P; i++) soma += pop[i].distancia_total;
            printf(" Geracao %d | Melhor: %.2f m | Media: %.2f m\n", geracao, melhor_global.distancia_total, soma / TAM_P);
        }

        if (geracoes_sem_melhora >= LIMITE_ESTAGNACAO) break;
    }

    clock_t fim_tempo = clock();
    *tempo_execucao_saida = (double)(fim_tempo - inicio_tempo) / CLOCKS_PER_SEC;
    *melhor_global_saida = melhor_global;
}

/* ------------------------------------------------------------ */
/* Main Orientado a Comparacao                                  */
/* ------------------------------------------------------------ */

int main(int argc, char *argv[]) {
    char nome_instancia[100];
    int total_geracoes;
    int taxa_mutacao_pop;
    int taxa_mutacao_gene;
    unsigned int semente;

    printf("--- Projeto de Rede FTTx - COMPARADOR AG Hibrido ---\n");

    if (argc >= 7) {
        strcpy(nome_instancia, argv[1]);
        total_geracoes = atoi(argv[2]);
        taxa_mutacao_pop = atoi(argv[3]);
        taxa_mutacao_gene = atoi(argv[4]);
        tipo_calculo = atoi(argv[5]);
        semente = (unsigned int)atoi(argv[6]);

        if (semente == 0) semente = (unsigned int)time(NULL);

        printf("\nModo Batch Detectado.\n");
    } else {
        printf("\nUso via terminal: ./comparador <instancia> <geracoes> <mut_pop> <mut_gene> <calc_dist> <semente>\n");
        printf("Iniciando modo interativo...\n\n");

        strcpy(nome_instancia, "novapalma128.txt");
        printf("Digite o nome da instancia (ex: cidade64.txt): ");
        scanf("%99s", nome_instancia);

        printf("Quantas geracoes deseja rodar? ");
        scanf("%d", &total_geracoes);

        printf("Taxa de mutacao da populacao (0 a 20%%, padrao %d%%): ", TAX_MUTACAO_POP_PADRAO);
        scanf("%d", &taxa_mutacao_pop);

        printf("Taxa de mutacao dos genes (1 a 5%%, padrao %d%%): ", TAX_MUTACAO_GENE_PADRAO);
        scanf("%d", &taxa_mutacao_gene);

        printf("Qual formula de distancia? (1 = Pitagoras, 2 = Haversine): ");
        scanf("%d", &tipo_calculo);

        printf("Use uma semente fixa para parear as execucoes (0 para relogio): ");
        scanf("%u", &semente);
        if (semente == 0) semente = (unsigned int)time(NULL);
    }

    printf("\nInstancia: %s | Semente Raiz: %u\n", nome_instancia, semente);

    ler_instancia_fttx(nome_instancia);
    preparar_grafo();

    printf("Clientes detectados: %d\n", total_clientes);

    struct individuo melhor_torneio, melhor_roleta;
    double tempo_torneio, tempo_roleta;

    printf("\n============================================\n");
    printf(" EXECUCAO 1: SELECAO POR TORNEIO\n");
    printf("============================================\n");
    executar_ag(1, total_geracoes, taxa_mutacao_pop, taxa_mutacao_gene, semente, &melhor_torneio, &tempo_torneio);

    printf("\n============================================\n");
    printf(" EXECUCAO 2: SELECAO POR ROLETA\n");
    printf("============================================\n");
    executar_ag(2, total_geracoes, taxa_mutacao_pop, taxa_mutacao_gene, semente, &melhor_roleta, &tempo_roleta);

    printf("\n============================================\n");
    printf(" RESULTADO COMPARATIVO FINAL\n");
    printf("============================================\n");
    printf("%-15s | %-15s | %-15s\n", "Metrica", "Torneio", "Roleta");
    printf("----------------|-----------------|-----------------\n");
    printf("%-15s | %12.2f m | %12.2f m\n", "Distancia", melhor_torneio.distancia_total, melhor_roleta.distancia_total);
    printf("%-15s | %12.4f s | %12.4f s\n", "Tempo Exec.", tempo_torneio, tempo_roleta);

    double diferenca = melhor_roleta.distancia_total - melhor_torneio.distancia_total;
    double proporcao = (melhor_roleta.distancia_total / melhor_torneio.distancia_total) * 100.0;

    printf("\nA Roleta gerou %.2f metros a mais que o Torneio.\n", diferenca);
    printf("Isso representa %.2f%% da metragem exigida pelo Torneio.\n", proporcao);

    FILE *csv = fopen("resultados_comparativo.csv", "a");
    if (csv != NULL) {
        fseek(csv, 0, SEEK_END);
        if (ftell(csv) == 0) {
            fprintf(csv, "Instancia,Geracoes,Mut_Pop,Mut_Gene,Semente,Distancia_Torneio,Tempo_Torneio,Distancia_Roleta,Tempo_Roleta\n");
        }
        fprintf(csv, "%s,%d,%d,%d,%u,%.2f,%.4f,%.2f,%.4f\n",
                nome_instancia, total_geracoes, taxa_mutacao_pop, taxa_mutacao_gene, semente,
                melhor_torneio.distancia_total, tempo_torneio,
                melhor_roleta.distancia_total, tempo_roleta);
        fclose(csv);
        printf("\nRegistro salvo com sucesso no arquivo 'resultados_comparativo.csv'.\n");
    }

    return 0;
}
