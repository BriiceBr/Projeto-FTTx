#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

#define RAIO_TERRA 6371000.0
#define PI 3.14159265358979323846

#define TAM_MAPA 367
#define TAM_P 100

/*
 * Mutacao em dois niveis:
 * - TAX_MUTACAO_POP_PADRAO: chance de um individuo ser selecionado para mutacao.
 * - TAX_MUTACAO_GENE_PADRAO: chance de cada gene desse individuo sofrer mutacao.
 */
#define TAX_MUTACAO_POP_PADRAO 20
#define TAX_MUTACAO_GENE_PADRAO 2
#define LIMITE_ESTAGNACAO 100000
#define MAX_TENTATIVAS_DUPLICATA 5

/* Heuristicas construtivas */
#define TOP_K_INICIAL 7
#define TOP_K_CRUZAMENTO 7
#define TOP_K_REPARO 7
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
    int categoria; /* 2 = comum aos dois pais, 1 = presente em um pai, 0 = nova */
    int fonte;     /* 1 = Pai A, 2 = Pai B, 3 = ambos, 0 = nenhuma */
    double distancia;
};

/* Areas de trabalho estaticas para evitar grande consumo da pilha. */
static struct candidato candidatos_trabalho[MAX_CANDIDATOS];

struct poste mapa[TAM_MAPA];
int matriz_arcos[TAM_MAPA][TAM_MAPA];

/* Distancias e vizinhanca sao precalculadas para evitar recomputacao. */
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

            matriz_distancias[i][j] =
                calcular_distancia(mapa[i], mapa[j]);

            if (tem_arco(i, j)) {
                vizinhos[i][grau_vizinho[i]++] = j;
            }
        }
    }
}

double distancia_aresta(int u, int v) {
    return matriz_distancias[u][v];
}

/* Verifica se a rua existe em determinada arvore parental. */
int possui_aresta_pai(const struct individuo *ind, int u, int v) {
    return (ind->pai[u] == v || ind->pai[v] == u);
}

/* Descobre em qual pai a aresta aparece. */
int fonte_aresta(const struct individuo *paiA,
                 const struct individuo *paiB,
                 int u, int v) {
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

    /* globais ja iniciadas em zero, mas zeramos para permitir reuso */
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

        if (strstr(linha, "Arc") != NULL) {
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
                    } else if (total_clientes > 0 &&
id_lido > (TAM_MAPA - total_clientes)) {
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

                if (origem >= 0 && origem < TAM_MAPA &&
                    destino >= 0 && destino < TAM_MAPA) {
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
        if (atual == no_mutado) {
            return 1;
        }
        atual = vetor_pais[atual];
        guard++;
    }

    /* Se a estrutura ja estiver corrompida, rejeitamos por seguranca. */
    if (guard >= TAM_MAPA) {
        return 1;
    }

    return 0;
}

/* Retorna 1 se um no esta na arvore ativa. */
int no_ativo(const struct individuo *ind, int no) {
    return (no == 0 || ind->pai[no] != -1);
}

/* ------------------------------------------------------------ */
/* Inicializacao: Prim guloso-probabilistico                     */
/* ------------------------------------------------------------ */

/*
 * Escolhe uma das K melhores arestas, mas nao todas com a mesma chance.
 * As arestas melhores recebem maior peso. Assim temos aleatoriedade
 * controlada, e nao aleatoriedade cega.
 */
int escolher_candidato_ponderado(struct candidato candidatos[], int n, int k) {
    int melhores[MAX_TOP_K_SELECAO];
    double melhores_dist[MAX_TOP_K_SELECAO];
    int quantidade = 0;

    if (n <= 0) {
        return -1;
    }

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
                int ti = melhores[pos];
                melhores[pos] = melhores[pos - 1];
                melhores[pos - 1] = ti;

                double td = melhores_dist[pos];
                melhores_dist[pos] = melhores_dist[pos - 1];
                melhores_dist[pos - 1] = td;
                pos--;
            }
        } else if (d < melhores_dist[quantidade - 1]) {
            int pos = quantidade - 1;
            melhores[pos] = i;
            melhores_dist[pos] = d;

            while (pos > 0 && melhores_dist[pos] < melhores_dist[pos - 1]) {
                int ti = melhores[pos];
                melhores[pos] = melhores[pos - 1];
                melhores[pos - 1] = ti;

                double td = melhores_dist[pos];
                melhores_dist[pos] = melhores_dist[pos - 1];
                melhores_dist[pos - 1] = td;
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

        if (sorteio <= acumulado) {
            return melhores[rank];
        }
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

    /* Pequenas variacoes de K criam diversidade sem usar arestas ruins aleatoriamente. */
    int k_inicial = 4 + (indice_populacao % 5); /* 4 a 8 */

    while (!todos_clientes_conectados(visitado)) {
        int num_candidatos = 0;

        for (int i = 0; i < TAM_MAPA; i++) {
            if (!visitado[i] || mapa[i].tipo == 2) {
                continue;
            }

            for (int pos = 0; pos < grau_vizinho[i]; pos++) {
                int j = vizinhos[i][pos];

                if (visitado[j]) {
                    continue;
                }

                candidatos[num_candidatos].origem = i;
                candidatos[num_candidatos].destino = j;
                candidatos[num_candidatos].categoria = 0;
                candidatos[num_candidatos].fonte = 0;
                candidatos[num_candidatos].distancia = distancia_aresta(i, j);
                num_candidatos++;
            }
        }

        if (num_candidatos == 0) {
            break;
        }

        int escolhido = escolher_candidato_ponderado(
            candidatos, num_candidatos, k_inicial);

        if (escolhido < 0) {
            break;
        }

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

        for (int i = 0; i < TAM_MAPA; i++) {
            filhos[i] = 0;
        }

        for (int i = 0; i < TAM_MAPA; i++) {
            if (ind->pai[i] != -1) {
                filhos[ind->pai[i]]++;
            }
        }

        for (int i = 1; i < TAM_MAPA; i++) {
            if (mapa[i].tipo == 1 &&
                ind->pai[i] != -1 &&
                filhos[i] == 0) {
                ind->pai[i] = -1;
                removeu = 1;
            }
        }
    } while (removeu);
}

int individuo_valido(const struct individuo *ind) {
    for (int i = 1; i < TAM_MAPA; i++) {
        if (ind->pai[i] == -1) {
            continue;
        }

        if (ind->pai[i] == i) {
            return 0;
        }

        if (!matriz_arcos[ind->pai[i]][i] &&
            !matriz_arcos[i][ind->pai[i]]) {
            return 0;
        }

        if (mapa[ind->pai[i]].tipo == 2) {
            return 0;
        }
    }

    /* Toda cadeia de pais de um no ativo precisa chegar a OLT. */
    for (int i = 1; i < TAM_MAPA; i++) {
        if (ind->pai[i] == -1) continue;

        int atual = i;
        int passos = 0;

        while (atual != 0) {
            if (atual < 0 || atual >= TAM_MAPA ||
                ind->pai[atual] == -1) {
                return 0;
            }
            atual = ind->pai[atual];
            passos++;
            if (passos > TAM_MAPA) {
                return 0;
            }
        }
    }

    /* Cada cliente precisa estar conectado. */
    for (int i = 0; i < TAM_MAPA; i++) {
        if (mapa[i].tipo == 2) {
            int atual = i;
            int passos = 0;

            while (atual != 0) {
                if (ind->pai[atual] == -1) {
                    return 0;
                }
                atual = ind->pai[atual];
                passos++;
                if (passos > TAM_MAPA) {
                    return 0;
                }
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

    if (pop_atual[s2].distancia_total < pop_atual[vencedor].distancia_total) {
        vencedor = s2;
    }
    if (pop_atual[s3].distancia_total < pop_atual[vencedor].distancia_total) {
        vencedor = s3;
    }

    return vencedor;
}

int roleta_indice(struct individuo pop_atual[]) {
    double soma_avaliacoes = 0.0;
    double fatias[TAM_P];

    for (int i = 0; i < TAM_P; i++) {
        if (pop_atual[i].distancia_total <= 0.0) {
            fatias[i] = 0.0;
        } else {
            fatias[i] = 1.0 / pop_atual[i].distancia_total;
        }
        soma_avaliacoes += fatias[i];
    }

    if (soma_avaliacoes <= 0.0) {
        return rand() % TAM_P;
    }

    double giro = ((double)rand() / (double)RAND_MAX) * soma_avaliacoes;
    double acumulador = 0.0;

    for (int i = 0; i < TAM_P; i++) {
        acumulador += fatias[i];
        if (acumulador >= giro) {
            return i;
        }
    }

    return TAM_P - 1;
}

/* ------------------------------------------------------------ */
/* Crossover genetico com preservacao de blocos                 */
/* ------------------------------------------------------------ */

/*
 * Acrescenta uma pequena subarvore do pai escolhido, a partir de um no
 * que acabou de ser conectado. Isso faz o crossover transmitir blocos
 * estruturais, em vez de reconstruir cada aresta do zero.
 */
int copiar_bloco_subarvore(const struct individuo *pai,
                           struct individuo *filho,
                           int raiz,
                           int visitado[],
                           int max_novos) {
    if (max_novos <= 0 || mapa[raiz].tipo == 2) {
        return 0;
    }

    int fila[TAM_MAPA];
    int inicio = 0;
    int fim = 0;
    int adicionados = 0;

    fila[fim++] = raiz;

    while (inicio < fim && adicionados < max_novos) {
        int atual = fila[inicio++];

        if (mapa[atual].tipo == 2) {
            continue;
        }

        for (int filho_no = 1; filho_no < TAM_MAPA; filho_no++) {
            if (pai->pai[filho_no] != atual) {
                continue;
            }

            if (visitado[filho_no]) {
                continue;
            }

            /* Aresta do pai precisa existir no grafo fisico. */
            if (!matriz_arcos[atual][filho_no] &&
                !matriz_arcos[filho_no][atual]) {
                continue;
            }

            filho->pai[filho_no] = atual;
            visitado[filho_no] = 1;
            adicionados++;

            if (adicionados >= max_novos) {
                break;
            }

            if (mapa[filho_no].tipo != 2) {
                fila[fim++] = filho_no;
            }

            if (fim >= TAM_MAPA) {
                break;
            }
        }
    }

    return adicionados;
}

int escolher_categoria_top_k(struct candidato candidatos[],
                                int n,
                                int categoria) {
    int melhores[MAX_TOP_K_SELECAO];
    double melhores_dist[MAX_TOP_K_SELECAO];
    int quantidade = 0;

    for (int i = 0; i < n; i++) {
        if (candidatos[i].categoria != categoria) {
            continue;
        }

        double d = candidatos[i].distancia;

        if (quantidade < TOP_K_CRUZAMENTO) {
            int p = quantidade;
            melhores[p] = i;
            melhores_dist[p] = d;
            quantidade++;

            while (p > 0 && melhores_dist[p] < melhores_dist[p - 1]) {
                int ti = melhores[p];
                melhores[p] = melhores[p - 1];
                melhores[p - 1] = ti;

                double td = melhores_dist[p];
                melhores_dist[p] = melhores_dist[p - 1];
                melhores_dist[p - 1] = td;
                p--;
            }
        } else if (d < melhores_dist[quantidade - 1]) {
            int p = quantidade - 1;
            melhores[p] = i;
            melhores_dist[p] = d;

            while (p > 0 && melhores_dist[p] < melhores_dist[p - 1]) {
                int ti = melhores[p];
                melhores[p] = melhores[p - 1];
                melhores[p - 1] = ti;

                double td = melhores_dist[p];
                melhores_dist[p] = melhores_dist[p - 1];
                melhores_dist[p - 1] = td;
                p--;
            }
        }
    }

    if (quantidade == 0) {
        return -1;
    }

    int soma_pesos = quantidade * (quantidade + 1) / 2;
    int sorteio = 1 + rand() % soma_pesos;
    int acumulado = 0;

    for (int rank = 0; rank < quantidade; rank++) {
        acumulado += quantidade - rank;
        if (sorteio <= acumulado) {
            return melhores[rank];
        }
    }

    return melhores[quantidade - 1];
}

struct individuo cruzamento_arvore(const struct individuo *paiA,
                                   const struct individuo *paiB) {
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

        /* Fronteira atual do filho. */
        for (int i = 0; i < TAM_MAPA; i++) {
            if (!visitado[i] || mapa[i].tipo == 2) {
                continue;
            }

            for (int pos = 0; pos < grau_vizinho[i]; pos++) {
                int j = vizinhos[i][pos];
                if (visitado[j]) {
                    continue;
                }

                int fonte = fonte_aresta(paiA, paiB, i, j);

                candidatos[num_candidatos].origem = i;
                candidatos[num_candidatos].destino = j;
                candidatos[num_candidatos].fonte = fonte;
                candidatos[num_candidatos].categoria = (fonte == 3) ? 2 :
                                                          (fonte != 0 ? 1 : 0);
                candidatos[num_candidatos].distancia =
                    distancia_aresta(i, j);
                num_candidatos++;
            }
        }

        if (num_candidatos == 0) {
            break;
        }

        int tem_comum = 0;
        int tem_herdada = 0;

        for (int i = 0; i < num_candidatos; i++) {
            if (candidatos[i].categoria == 2) tem_comum = 1;
            if (candidatos[i].categoria == 1) tem_herdada = 1;
        }

        int indice = -1;
        int categoria_escolhida;
        int sorteio = rand() % 100;

        /*
         * Prioridade genetica:
         * 70% = aresta comum aos dois pais
         * 25% = aresta exclusiva de um dos pais
         *  5% = aresta nova/reparo
         *
         * Quando uma categoria nao existe, usamos a proxima disponivel.
         */
        if (tem_comum && sorteio < 70) {
            categoria_escolhida = 2;
            indice = escolher_categoria_top_k(candidatos,
                                               num_candidatos,
                                               categoria_escolhida);
        }

        if (indice == -1 && tem_herdada && sorteio >= 70 && sorteio < 95) {
            categoria_escolhida = 1;
            indice = escolher_categoria_top_k(candidatos,
                                               num_candidatos,
                                               categoria_escolhida);
        }

        if (indice == -1 && sorteio >= 95) {
            categoria_escolhida = 0;
            indice = escolher_categoria_top_k(candidatos,
                                               num_candidatos,
                                               categoria_escolhida);
        }

        /* Fallback quando a faixa sorteada nao tem candidatos. */
        if (indice == -1 && tem_comum) {
            indice = escolher_categoria_top_k(candidatos, num_candidatos, 2);
        }

        if (indice == -1 && tem_herdada) {
            indice = escolher_categoria_top_k(candidatos, num_candidatos, 1);
        }

        if (indice == -1) {
            /* Reparo: escolhe de forma ponderada entre as melhores arestas. */
            indice = escolher_candidato_ponderado(
                candidatos, num_candidatos, TOP_K_REPARO);
        }

        int origem = candidatos[indice].origem;
        int destino = candidatos[indice].destino;
        int fonte = candidatos[indice].fonte;

        filho.pai[destino] = origem;
        visitado[destino] = 1;

        /*
         * BLOCO GENETICO:
         * se a aresta veio de um pai, tenta copiar alguns descendentes
         * desse mesmo pai. O tamanho do bloco e pequeno para preservar
         * diversidade e evitar simplesmente clonar o pai.
         */
        if (fonte != 0 && mapa[destino].tipo != 2) {
            const struct individuo *fonte_pai = NULL;

            if (fonte == 1) {
                fonte_pai = paiA;
            } else if (fonte == 2) {
                fonte_pai = paiB;
            } else {
                fonte_pai = (rand() % 2 == 0) ? paiA : paiB;
            }

            int tamanho_bloco = 1 + rand() % TAM_BLOCO;
            copiar_bloco_subarvore(fonte_pai,
                                   &filho,
                                   destino,
                                   visitado,
                                   tamanho_bloco);
        }
    }

    return filho;
}

/* ------------------------------------------------------------ */
/* Mutacao: taxa da populacao + taxa dos genes                  */
/* ------------------------------------------------------------ */

void mutacao_arvore(struct individuo *ind,
                    int taxa_mutacao_pop,
                    int taxa_mutacao_gene) {
    if (taxa_mutacao_pop <= 0 || taxa_mutacao_gene <= 0) {
        return;
    }

    if (taxa_mutacao_pop > 100) taxa_mutacao_pop = 100;
    if (taxa_mutacao_gene > 100) taxa_mutacao_gene = 100;

    /* Primeiro: decide se este individuo sofre mutacao. */
    if ((rand() % 100) >= taxa_mutacao_pop) {
        return;
    }

    /* Segundo: cada gene ativo tem sua propria chance de mutar. */
    for (int no_mutado = 1; no_mutado < TAM_MAPA; no_mutado++) {
        if (ind->pai[no_mutado] == -1) {
            continue;
        }

        if ((rand() % 100) >= taxa_mutacao_gene) {
            continue;
        }

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
            distancias_validas[quantidade] =
                distancia_aresta(candidato, no_mutado);
            quantidade++;
        }

        if (quantidade == 0) {
            continue;
        }

        /* Reaproveita a ideia de TOP-K, mas sem ordenar toda a lista. */
        int limite = quantidade;
        if (limite > TOP_K_REPARO) {
            limite = TOP_K_REPARO;
        }

        int melhores[TOP_K_REPARO];
        double melhores_dist[TOP_K_REPARO];
        int qtd_melhores = 0;

        for (int i = 0; i < quantidade; i++) {
            if (qtd_melhores < limite) {
                int p = qtd_melhores;
                melhores[p] = vizinhos_validos[i];
                melhores_dist[p] = distancias_validas[i];
                qtd_melhores++;

                while (p > 0 && melhores_dist[p] < melhores_dist[p - 1]) {
                    int ti = melhores[p];
                    melhores[p] = melhores[p - 1];
                    melhores[p - 1] = ti;

                    double td = melhores_dist[p];
                    melhores_dist[p] = melhores_dist[p - 1];
                    melhores_dist[p - 1] = td;
                    p--;
                }
            } else if (distancias_validas[i] < melhores_dist[qtd_melhores - 1]) {
                int p = qtd_melhores - 1;
                melhores[p] = vizinhos_validos[i];
                melhores_dist[p] = distancias_validas[i];

                while (p > 0 && melhores_dist[p] < melhores_dist[p - 1]) {
                    int ti = melhores[p];
                    melhores[p] = melhores[p - 1];
                    melhores[p - 1] = ti;

                    double td = melhores_dist[p];
                    melhores_dist[p] = melhores_dist[p - 1];
                    melhores_dist[p - 1] = td;
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

/* ------------------------------------------------------------ */
/* Diversidade da populacao                                    */
/* ------------------------------------------------------------ */

unsigned long long hash_individuo(const struct individuo *ind) {
    unsigned long long hash = 1469598103934665603ULL;

    for (int i = 0; i < TAM_MAPA; i++) {
        unsigned long long valor =
            (unsigned long long)(ind->pai[i] + 2);
        hash ^= valor + (unsigned long long)(i + 1) * 1099511628211ULL;
        hash *= 1099511628211ULL;
    }

    return hash;
}

int individuo_igual(const struct individuo *a,
                    const struct individuo *b) {
    for (int i = 0; i < TAM_MAPA; i++) {
        if (a->pai[i] != b->pai[i]) {
            return 0;
        }
    }
    return 1;
}

int individuo_ja_existe(const struct individuo pop_atual[],
                        const unsigned long long hashes[],
                        int quantidade,
                        const struct individuo *ind,
                        unsigned long long hash) {
    for (int i = 0; i < quantidade; i++) {
        if (hashes[i] == hash && individuo_igual(&pop_atual[i], ind)) {
            return 1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------ */
/* Diversidade                                                   */
/* ------------------------------------------------------------ */

double calcular_diversidade_unica(struct individuo pop_atual[]) {
    int unicos = 0;

    for (int i = 0; i < TAM_P; i++) {
        int repetido = 0;

        for (int j = 0; j < i; j++) {
            if (individuo_igual(&pop_atual[i], &pop_atual[j])) {
                repetido = 1;
                break;
            }
        }

        if (!repetido) {
            unicos++;
        }
    }

    return 100.0 * (double)unicos / (double)TAM_P;
}

/* Mede quanto, em media, os genes da populacao diferem da elite. */
double calcular_diversidade_genetica(struct individuo pop_atual[],
                                      int indice_elite) {
    long long diferentes = 0;
    long long total_genes = (long long)TAM_P * (long long)TAM_MAPA;

    for (int i = 0; i < TAM_P; i++) {
        for (int j = 0; j < TAM_MAPA; j++) {
            if (pop_atual[i].pai[j] != pop_atual[indice_elite].pai[j]) {
                diferentes++;
            }
        }
    }

    if (total_genes == 0) {
        return 0.0;
    }

    return 100.0 * (double)diferentes / (double)total_genes;
}

/* ------------------------------------------------------------ */
/* Visualizador Python temporario                               */
/* ------------------------------------------------------------ */

void abrir_visualizador_python(const char nome_instancia[],
                              const struct individuo *melhor_global,
                              const double geracoes[],
                              const double melhores[],
                              const double medias[],
                              const double diversidades_unicas[],
                              const double diversidades_geneticas[],
                              int quantidade_historico,
                              double tempo_execucao) {
    char nome_script[128];
    char comando[256];

    snprintf(nome_script, sizeof(nome_script),
             "visualizador_fttx_temp_%u.py", (unsigned int)time(NULL));

    FILE *py = fopen(nome_script, "w");
    if (py == NULL) {
        printf("\n-> Nao foi possivel criar o visualizador Python temporario.\n");
        return;
    }

    fprintf(py, "import matplotlib.pyplot as plt\n");
    fprintf(py, "import networkx as nx\n\n");
    fprintf(py, "nome_instancia = '%s'\n", nome_instancia);
    fprintf(py, "melhor_distancia = %.12f\n", melhor_global->distancia_total);
    fprintf(py, "tempo_execucao = %.12f\n\n", tempo_execucao);

    fprintf(py, "posicoes = {\n");
    for (int i = 0; i < TAM_MAPA; i++) {
        fprintf(py, "    %d: (%.12f, %.12f),\n",
                mapa[i].id + 1,
                mapa[i].y,
                mapa[i].x);
    }
    fprintf(py, "}\n\n");

    fprintf(py, "lista_olt = [");
    for (int i = 0; i < TAM_MAPA; i++) {
        if (mapa[i].tipo == 0) fprintf(py, "%d,", mapa[i].id + 1);
    }
    fprintf(py, "]\n");

    fprintf(py, "lista_postes = [");
    for (int i = 0; i < TAM_MAPA; i++) {
        if (mapa[i].tipo == 1) fprintf(py, "%d,", mapa[i].id + 1);
    }
    fprintf(py, "]\n");

    fprintf(py, "lista_clientes = [");
    for (int i = 0; i < TAM_MAPA; i++) {
        if (mapa[i].tipo == 2) fprintf(py, "%d,", mapa[i].id + 1);
    }
    fprintf(py, "]\n\n");

    fprintf(py, "conexoes = [\n");
    for (int i = 0; i < TAM_MAPA; i++) {
        if (melhor_global->pai[i] != -1 &&
            melhor_global->pai[i] != i) {
            fprintf(py, "    (%d, %d),\n",
                    melhor_global->pai[i] + 1,
                    i + 1);
        }
    }
    fprintf(py, "]\n\n");

    fprintf(py, "geracoes = [");
    for (int i = 0; i < quantidade_historico; i++) {
        fprintf(py, "%.0f,", geracoes[i]);
    }
    fprintf(py, "]\n");

    fprintf(py, "melhores = [");
    for (int i = 0; i < quantidade_historico; i++) {
        fprintf(py, "%.12f,", melhores[i]);
    }
    fprintf(py, "]\n");

    fprintf(py, "medias = [");
    for (int i = 0; i < quantidade_historico; i++) {
        fprintf(py, "%.12f,", medias[i]);
    }
    fprintf(py, "]\n");

    fprintf(py, "diversidades_unicas = [");
    for (int i = 0; i < quantidade_historico; i++) {
        fprintf(py, "%.12f,", diversidades_unicas[i]);
    }
    fprintf(py, "]\n");

    fprintf(py, "diversidades_geneticas = [");
    for (int i = 0; i < quantidade_historico; i++) {
        fprintf(py, "%.12f,", diversidades_geneticas[i]);
    }
    fprintf(py, "]\n\n");

    fprintf(py, "G = nx.Graph()\n");
    fprintf(py, "G.add_nodes_from(posicoes.keys())\n");
    fprintf(py, "G.add_edges_from(conexoes)\n\n");

    fprintf(py, "plt.figure(figsize=(14, 8))\n");
    fprintf(py, "nx.draw_networkx_nodes(G, pos=posicoes, nodelist=lista_olt, node_color='orange', edgecolors='black', node_size=80, label='OLT')\n");
    fprintf(py, "nx.draw_networkx_nodes(G, pos=posicoes, nodelist=lista_postes, node_color='blue', alpha=0.7, node_size=30, label='Poste')\n");
    fprintf(py, "nx.draw_networkx_nodes(G, pos=posicoes, nodelist=lista_clientes, node_color='crimson', node_size=35, label='Cliente')\n");
    fprintf(py, "nx.draw_networkx_edges(G, pos=posicoes, edgelist=conexoes, width=1.2, edge_color='black')\n");
    fprintf(py, "plt.title(f'Instancia: {nome_instancia} | Melhor: {melhor_distancia:.2f} m | Tempo: {tempo_execucao:.2f} s', fontsize=11)\n");
    fprintf(py, "plt.legend(loc='lower right', scatterpoints=1)\n");
    fprintf(py, "plt.axis('off')\n");
    fprintf(py, "plt.tight_layout()\n\n");

    fprintf(py, "plt.figure(figsize=(10, 5))\n");
    fprintf(py, "plt.plot(geracoes, melhores, label='Melhor individuo')\n");
    fprintf(py, "plt.plot(geracoes, medias, label='Media da populacao')\n");
    fprintf(py, "plt.xlabel('Geracao')\n");
    fprintf(py, "plt.ylabel('Distancia (m)')\n");
    fprintf(py, "plt.title('Evolucao do custo da populacao')\n");
    fprintf(py, "plt.grid(True, alpha=0.3)\n");
    fprintf(py, "plt.legend()\n");
    fprintf(py, "plt.tight_layout()\n\n");

    fprintf(py, "plt.figure(figsize=(10, 5))\n");
    fprintf(py, "plt.plot(geracoes, diversidades_unicas, label='Individuos unicos (%%)')\n");
    fprintf(py, "plt.plot(geracoes, diversidades_geneticas, label='Genes diferentes da elite (%%)')\n");
    fprintf(py, "plt.ylim(0, 100)\n");
    fprintf(py, "plt.xlabel('Geracao')\n");
    fprintf(py, "plt.ylabel('Diversidade (%%)')\n");
    fprintf(py, "plt.title('Diversidade da populacao')\n");
    fprintf(py, "plt.grid(True, alpha=0.3)\n");
    fprintf(py, "plt.legend()\n");
    fprintf(py, "plt.tight_layout()\n");
    fprintf(py, "plt.show()\n");

    fclose(py);

    snprintf(comando, sizeof(comando), "py \"%s\"", nome_script);
    int retorno = system(comando);

    if (retorno != 0) {
        snprintf(comando, sizeof(comando), "python \"%s\"", nome_script);
        retorno = system(comando);
    }

    if (retorno != 0) {
        snprintf(comando, sizeof(comando), "python3 \"%s\"", nome_script);
        retorno = system(comando);
    }

    if (retorno != 0) {
        printf("\n-> Python nao conseguiu abrir o visualizador.\n");
        printf("   Instale Python, matplotlib e networkx para usar o visualizador.\n");
    }

    remove(nome_script);
}

/* ------------------------------------------------------------ */
/* Melhor individuo                                             */
/* ------------------------------------------------------------ */

int indice_melhor(struct individuo pop_atual[]) {
    int melhor = 0;

    for (int i = 1; i < TAM_P; i++) {
        if (pop_atual[i].distancia_total <
            pop_atual[melhor].distancia_total) {
            melhor = i;
        }
    }

    return melhor;
}

/* ------------------------------------------------------------ */
/* Main                                                          */
/* ------------------------------------------------------------ */

int main(int argc, char *argv[]) {
    unsigned int semente;
    int total_geracoes;
    int metodo_selecao;
    int taxa_mutacao_pop;
    int taxa_mutacao_gene;

    char nome_instancia[100] = "cidade64.txt";
    char nome_log[150];

    printf("--- Projeto de Rede FTTx - AG Hibrido ---\n\n");

    if (argc > 1) {
        strncpy(nome_instancia, argv[1], sizeof(nome_instancia) - 1);
        nome_instancia[sizeof(nome_instancia) - 1] = '\0';
    } else {
        printf("Digite o nome da instancia (ex: cidade64.txt, interior64.txt, novapalma128.txt, toy3.txt) [padrao: cidade64.txt]: ");
        char buffer[100];
        if (fgets(buffer, sizeof(buffer), stdin) && buffer[0] != '\n' && buffer[0] != '\r') {
            buffer[strcspn(buffer, "\r\n")] = '\0';
            if (strlen(buffer) > 0) {
                strncpy(nome_instancia, buffer, sizeof(nome_instancia) - 1);
                nome_instancia[sizeof(nome_instancia) - 1] = '\0';
            }
        }
    }
    printf("Instancia selecionada: %s\n", nome_instancia);

    ler_instancia_fttx(nome_instancia);

    printf("Clientes detectados: %d\n", total_clientes);

    printf("Quantas geracoes deseja rodar? ");
    if (scanf("%d", &total_geracoes) != 1 || total_geracoes <= 0) {
        printf("Numero de geracoes invalido.\n");
        return 1;
    }

    printf("Qual metodo de selecao deseja usar? (1 = Torneio, 2 = Roleta): ");
    if (scanf("%d", &metodo_selecao) != 1 ||
        (metodo_selecao != 1 && metodo_selecao != 2)) {
        printf("Metodo de selecao invalido.\n");
        return 1;
    }

    printf("Taxa de mutacao da populacao (0 a 20%%, padrao %d%%): ", TAX_MUTACAO_POP_PADRAO);
    if (scanf("%d", &taxa_mutacao_pop) != 1 ||
        taxa_mutacao_pop < 0 || taxa_mutacao_pop > 20) {
        printf("Taxa de mutacao da populacao invalida.\n");
        return 1;
    }

    printf("Taxa de mutacao dos genes (1 a 5%%, padrao %d%%): ", TAX_MUTACAO_GENE_PADRAO);
    if (scanf("%d", &taxa_mutacao_gene) != 1 ||
        taxa_mutacao_gene < 1 || taxa_mutacao_gene > 5) {
        printf("Taxa de mutacao dos genes invalida.\n");
        return 1;
    }

    printf("Qual formula de distancia deseja aplicar? (1 = Pitagoras, 2 = Haversine): ");
    if (scanf("%d", &tipo_calculo) != 1 ||
        (tipo_calculo != 1 && tipo_calculo != 2)) {
        printf("Formula de distancia invalida.\n");
        return 1;
    }

    printf("Use uma semente fixa para repetir um experimento ou 0 para usar o relogio: ");
    if (scanf("%u", &semente) != 1) {
        printf("Semente invalida.\n");
        return 1;
    }

    if (semente == 0) {
        semente = (unsigned int)time(NULL);
    }

    srand(semente);
    preparar_grafo();

    int capacidade_historico = total_geracoes + 1;
    double *hist_geracoes = malloc((size_t)capacidade_historico * sizeof(double));
    double *hist_melhores = malloc((size_t)capacidade_historico * sizeof(double));
    double *hist_medias = malloc((size_t)capacidade_historico * sizeof(double));
    double *hist_div_unicas = malloc((size_t)capacidade_historico * sizeof(double));
    double *hist_div_geneticas = malloc((size_t)capacidade_historico * sizeof(double));

    if (hist_geracoes == NULL || hist_melhores == NULL ||
        hist_medias == NULL || hist_div_unicas == NULL ||
        hist_div_geneticas == NULL) {
        printf("Erro: memoria insuficiente para o historico.\n");
        free(hist_geracoes);
        free(hist_melhores);
        free(hist_medias);
        free(hist_div_unicas);
        free(hist_div_geneticas);
        return 1;
    }

    int quantidade_historico = 0;

    printf("\nIniciando a evolucao...\n");
    printf("Populacao: %d | Geracoes: %d | Mutacao populacao: %d%% | Mutacao genes: %d%%\n",
           TAM_P, total_geracoes, taxa_mutacao_pop, taxa_mutacao_gene);
    printf("Selecao: %s | Distancia: %s\n\n",
           metodo_selecao == 1 ? "Torneio" : "Roleta",
           tipo_calculo == 1 ? "Pitagoras" : "Haversine");

    clock_t inicio_tempo = clock();

    /* Populacao inicial: evita copias identicas quando possivel. */
    unsigned long long hashes_pop[TAM_P];

    for (int i = 0; i < TAM_P; i++) {
        int gerado = 0;

        for (int tentativa = 0; tentativa < MAX_TENTATIVAS_DUPLICATA; tentativa++) {
            gerar_primeira_pop(&pop[i], i + tentativa);
            avaliar_individuo(&pop[i]);

            if (!individuo_valido(&pop[i])) {
                continue;
            }

            unsigned long long hash = hash_individuo(&pop[i]);

            if (!individuo_ja_existe(pop, hashes_pop, i, &pop[i], hash)) {
                hashes_pop[i] = hash;
                gerado = 1;
                break;
            }
        }

        if (!gerado) {
            printf("ERRO: nao foi possivel gerar individuo inicial valido/diverso (%d).\n", i);
            return 1;
        }
    }

    int melhor_inicial = indice_melhor(pop);
    struct individuo melhor_global = pop[melhor_inicial];
    double melhor_historico = melhor_global.distancia_total;
    int geracoes_sem_melhora = 0;

    {
        double soma = 0.0;
        for (int i = 0; i < TAM_P; i++) {
            soma += pop[i].distancia_total;
        }

        hist_geracoes[quantidade_historico] = 0.0;
        hist_melhores[quantidade_historico] = melhor_global.distancia_total;
        hist_medias[quantidade_historico] = soma / TAM_P;
        hist_div_unicas[quantidade_historico] = calcular_diversidade_unica(pop);
        hist_div_geneticas[quantidade_historico] =
            calcular_diversidade_genetica(pop, melhor_inicial);
        quantidade_historico++;
    }

    printf("Melhor individuo inicial: %.2f metros\n", melhor_historico);
    printf("Diversidade inicial: %.2f%% individuos unicos | %.2f%% genes diferentes da elite\n\n",
           hist_div_unicas[0], hist_div_geneticas[0]);

    for (int geracao = 1; geracao <= total_geracoes; geracao++) {
        /* Gera filhos; a elite entra primeiro e os demais tentam ser diferentes. */
        unsigned long long hashes_nova[TAM_P];
        hashes_nova[0] = hash_individuo(&melhor_global);
        pop_nova[0] = melhor_global;

        for (int i = 1; i < TAM_P; i++) {
            struct individuo filho = melhor_global;
            int filho_aceito = 0;

            for (int tentativa = 0;
                 tentativa < MAX_TENTATIVAS_DUPLICATA;
                 tentativa++) {
                int indice_paiA;
                int indice_paiB;

                if (metodo_selecao == 1) {
                    indice_paiA = torneio_indice(pop);
                    indice_paiB = torneio_indice(pop);
                } else {
                    indice_paiA = roleta_indice(pop);
                    indice_paiB = roleta_indice(pop);
                }

                /* Evita cruzar o individuo consigo mesmo sempre que possivel. */
                for (int tentativa_pai = 0;
                     tentativa_pai < 3 && indice_paiB == indice_paiA;
                     tentativa_pai++) {
                    if (metodo_selecao == 1) {
                        indice_paiB = torneio_indice(pop);
                    } else {
                        indice_paiB = roleta_indice(pop);
                    }
                }

                filho = cruzamento_arvore(&pop[indice_paiA],
                                          &pop[indice_paiB]);

                mutacao_arvore(&filho,
                               taxa_mutacao_pop,
                               taxa_mutacao_gene);
                avaliar_individuo(&filho);

                if (!individuo_valido(&filho)) {
                    continue;
                }

                unsigned long long hash = hash_individuo(&filho);

                if (!individuo_ja_existe(pop_nova,
                                         hashes_nova,
                                         i,
                                         &filho,
                                         hash)) {
                    hashes_nova[i] = hash;
                    pop_nova[i] = filho;
                    filho_aceito = 1;
                    break;
                }
            }

            /* Em ultimo caso, aceita uma solucao valida mesmo que seja repetida. */
            if (!filho_aceito) {
                pop_nova[i] = filho;
                hashes_nova[i] = hash_individuo(&pop_nova[i]);
            }
        }

        /* Elitismo + melhor filho da geracao. */
        int melhor_geracao = indice_melhor(pop_nova);

        if (pop_nova[melhor_geracao].distancia_total <
            melhor_global.distancia_total) {
            melhor_global = pop_nova[melhor_geracao];
            melhor_historico = melhor_global.distancia_total;
            geracoes_sem_melhora = 0;
        } else {
            geracoes_sem_melhora++;
        }

        /* Garante que a melhor solucao global continue na populacao. */
        pop_nova[0] = melhor_global;

        for (int i = 0; i < TAM_P; i++) {
            pop[i] = pop_nova[i];
        }

        if (quantidade_historico < capacidade_historico) {
            int melhor_atual = indice_melhor(pop);
            double soma = 0.0;

            for (int i = 0; i < TAM_P; i++) {
                soma += pop[i].distancia_total;
            }

            hist_geracoes[quantidade_historico] = (double)geracao;
            hist_melhores[quantidade_historico] =
                pop[melhor_atual].distancia_total;
            hist_medias[quantidade_historico] = soma / TAM_P;
            hist_div_unicas[quantidade_historico] = calcular_diversidade_unica(pop);
            hist_div_geneticas[quantidade_historico] =
                calcular_diversidade_genetica(pop, melhor_atual);
            quantidade_historico++;
        }

        if (geracao % 100 == 0 || geracao == 1) {
            int atual_melhor = indice_melhor(pop);
            double soma = 0.0;
            for (int i = 0; i < TAM_P; i++) {
                soma += pop[i].distancia_total;
            }

            printf("Geracao %d | Melhor: %.2f m | Media: %.2f m | Unicos: %.2f%% | Genes diferentes: %.2f%%\n",
                   geracao,
                   pop[atual_melhor].distancia_total,
                   soma / TAM_P,
                   hist_div_unicas[quantidade_historico - 1],
                   hist_div_geneticas[quantidade_historico - 1]);
        }

        if (geracoes_sem_melhora >= LIMITE_ESTAGNACAO) {
            printf("\n>>> Paragem antecipada na geracao %d: %d geracoes sem melhora.\n",
                   geracao, geracoes_sem_melhora);
            break;
        }
    }

    clock_t fim_tempo = clock();
    double tempo_execucao =
        (double)(fim_tempo - inicio_tempo) / CLOCKS_PER_SEC;

    printf("\n============================================\n");
    printf("RESULTADO FINAL\n");
    printf("============================================\n");
    printf("Melhor distancia encontrada: %.2f metros\n",
           melhor_global.distancia_total);
    printf("Tempo computacional: %.6f segundos\n", tempo_execucao);
    printf("Semente utilizada: %u\n", semente);
    if (quantidade_historico > 0) {
        printf("Diversidade final: %.2f%% individuos unicos | %.2f%% genes diferentes da elite\n",
               hist_div_unicas[quantidade_historico - 1],
               hist_div_geneticas[quantidade_historico - 1]);
    }

    printf("\n--- MAPA DE CONEXOES (MELHOR INDIVIDUO) ---\n");

    for (int k = 0; k < TAM_MAPA; k++) {
        if (melhor_global.pai[k] != -1 &&
            melhor_global.pai[k] != k) {
            printf(" -> Cabo saindo do No %d ate o No %d\n",
                   melhor_global.pai[k] + 1,
                   k + 1);
        }
    }

    printf("--------------------------------------------\n");

    sprintf(nome_log, "log_%s", nome_instancia);

    FILE *log = fopen(nome_log, "a");
    if (log != NULL) {
        fprintf(log, "========================================\n");
        fprintf(log, "RELATORIO DE EXECUCAO - AG HIBRIDO FTTX\n");
        fprintf(log, "========================================\n");
        fprintf(log, "Arquivo analisado: %s\n", nome_instancia);
        fprintf(log, "Tamanho da populacao: %d\n", TAM_P);
        fprintf(log, "Total de geracoes: %d\n", total_geracoes);
        fprintf(log, "Taxa de mutacao da populacao: %d%%\n", taxa_mutacao_pop);
        fprintf(log, "Taxa de mutacao dos genes: %d%%\n", taxa_mutacao_gene);
        fprintf(log, "Limite de estagnacao: %d geracoes\n", LIMITE_ESTAGNACAO);
        fprintf(log, "Selecao: %s\n",
                metodo_selecao == 1 ? "Torneio" : "Roleta");
        fprintf(log, "Distancia: %s\n",
                tipo_calculo == 1 ? "Pitagoras" : "Haversine");
        fprintf(log, "Semente: %u\n", semente);
        fprintf(log, "\nRESULTADOS:\n");
        fprintf(log, "Menor distancia absoluta encontrada: %.2f metros\n",
                melhor_global.distancia_total);
        fprintf(log, "Tempo computacional: %.6f segundos\n",
                tempo_execucao);
        if (quantidade_historico > 0) {
            fprintf(log, "Diversidade final (individuos unicos): %.2f%%\n",
                    hist_div_unicas[quantidade_historico - 1]);
            fprintf(log, "Diversidade final (genes diferentes da elite): %.2f%%\n",
                    hist_div_geneticas[quantidade_historico - 1]);
        }
        fprintf(log, "----------------------------------------\n");
        fprintf(log, "MAPA DE CONEXOES (ELITE)\n");

        for (int j = 0; j < TAM_MAPA; j++) {
            if (melhor_global.pai[j] != -1 &&
                melhor_global.pai[j] != j) {
                fprintf(log,
                        "-> Cabo saindo do No %d ate o No %d\n",
                        melhor_global.pai[j] + 1,
                        j + 1);
            }
        }

        fprintf(log, "\n");
        fclose(log);
        printf("\n-> Log salvo com sucesso em '%s'.\n", nome_log);
    } else {
        printf("\n-> Atencao: nao foi possivel criar o log.\n");
    }

    printf("\n-> Abrindo visualizador Python automaticamente...\n");
    abrir_visualizador_python(nome_instancia,
                              &melhor_global,
                              hist_geracoes,
                              hist_melhores,
                              hist_medias,
                              hist_div_unicas,
                              hist_div_geneticas,
                              quantidade_historico,
                              tempo_execucao);

    free(hist_geracoes);
    free(hist_melhores);
    free(hist_medias);
    free(hist_div_unicas);
    free(hist_div_geneticas);

    return 0;
}
