#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<math.h>
#include<string.h>

#define MAX_NOS 400
#define TAM_P 100
#define PI 3.14159265358979323846

int nos_reais = 0;
int taxa_mutacao_atual;

struct poste {
    int id;
    float x;
    float y;
    int tipo;
};

struct individuo {
    int pai[MAX_NOS];
    double distancia_total;
    double atenuacao_maxima;
};

struct aresta {
    int u;
    int v;
};

struct poste mapa[MAX_NOS];
int matriz_arcos[MAX_NOS][MAX_NOS];

// Memoria global blindada contra Stack Overflow
struct aresta lista_arestas[30000];
int total_arestas = 0;
int cand_origem[30000];
int cand_destino[30000];
int num_candidatos = 0;

struct individuo pop[TAM_P];
struct individuo pop_nova[TAM_P];

void limpar_memoria() {
    int i, j;
    for(i = 0; i < MAX_NOS; i++) {
        mapa[i].tipo = 0;
        for(j = 0; j < MAX_NOS; j++) {
            matriz_arcos[i][j] = 0;
        }
    }
    total_arestas = 0;
}

void ler_instancia_fttx(char nome_arq[]) {
    limpar_memoria();

    FILE* arq = fopen(nome_arq, "r");
    if (arq == NULL) {
        return;
    }
    char linhas[256];
    int lendo_nos = 0;
    int lendo_arcos = 0;
    int num_clientes = 0;

    while (fgets(linhas, sizeof(linhas), arq)) {
        if (strstr(linhas, "Clients") != NULL) {
            sscanf(linhas, "Clients %d", &num_clientes);
            continue;
        }

        if (strstr(linhas, "Nodes") != NULL) {
            sscanf(linhas, "Nodes %d", &nos_reais);
            lendo_nos = 1;
            lendo_arcos = 0;
            continue;
        }

        if (strstr(linhas, "Arcs") != NULL || strstr(linhas, "Arc ") != NULL) {
            lendo_nos = 0;
            lendo_arcos = 1;
            continue;
        }

        if (strstr(linhas, "Splitters") != NULL || strstr(linhas, "BalancedSplitters") != NULL) {
            lendo_arcos = 0;
        }

        if (lendo_nos == 1) {
            int id_lido;
            float x, y;

            if (sscanf(linhas, "%d %f %f", &id_lido, &x, &y) == 3) {
                int id_correto = id_lido - 1;
                mapa[id_correto].id = id_correto;
                mapa[id_correto].x = x;
                mapa[id_correto].y = y;

                if (id_lido == 1) {
                    mapa[id_correto].tipo = 0;
                }
                else if (num_clientes > 0 && id_lido > (nos_reais - num_clientes)) {
                    mapa[id_correto].tipo = 2;
                }
                else {
                    mapa[id_correto].tipo = 1;
                }
            }
        }

        if (lendo_arcos == 1) {
            int origem, destino;
            if (sscanf(linhas, "%d %d", &origem, &destino) == 2) {
                matriz_arcos[origem - 1][destino - 1] = 1;
                // Guarda o endereço exato da rua para otimizar a velocidade
                lista_arestas[total_arestas].u = origem - 1;
                lista_arestas[total_arestas].v = destino - 1;
                total_arestas++;
            }
        }
    }
    fclose(arq);
}

void gerar_primeira_pop(struct individuo* ind) {
    int i;
    int visitado[MAX_NOS];

    for(i = 0; i < nos_reais; i++) {
        ind->pai[i] = -1;
        visitado[i] = 0;
    }

    visitado[0] = 1;
    ind->pai[0] = -1;

    int nos_conectados = 1;

    while (nos_conectados < nos_reais) {
        num_candidatos = 0;

        // Otimizacao relampago: varre apenas as ruas que existem
        for (int e = 0; e < total_arestas; e++) {
            int o = lista_arestas[e].u;
            int d = lista_arestas[e].v;
            if (visitado[o] == 1 && visitado[d] == 0 && mapa[o].tipo != 2) {
                cand_origem[num_candidatos] = o;
                cand_destino[num_candidatos] = d;
                num_candidatos++;
            }
        }

        if (num_candidatos == 0) {
            break;
        }

        int sorteio = rand() % num_candidatos;
        int origem_escolhida = cand_origem[sorteio];
        int destino_escolhido = cand_destino[sorteio];

        ind->pai[destino_escolhido] = origem_escolhida;
        visitado[destino_escolhido] = 1;
        nos_conectados++;
    }
}

// Calcula a distancia real em metros entre duas coordenadas de GPS
double pit(struct poste p1, struct poste p2) {
    // Converte as latitudes e longitudes de graus para radianos
    double lat1 = p1.x * PI / 180.0;
    double lon1 = p1.y * PI / 180.0;
    double lat2 = p2.x * PI / 180.0;
    double lon2 = p2.y * PI / 180.0;

    // Diferencas
    double dlat = lat2 - lat1;
    double dlon = lon2 - lon1;

    // Formula de Haversine
    double a = sin(dlat / 2) * sin(dlat / 2) +
               cos(lat1) * cos(lat2) *
               sin(dlon / 2) * sin(dlon / 2);

    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    // Raio da Terra em metros
    double raio_terra = 6371000.0;

    return raio_terra * c; // Retorna a distancia real da rua em metros!
}

void avaliar_individuo(struct individuo *ind) {
    int i;
    int filhos[MAX_NOS];
    int removeu_alguem;

    do {
        removeu_alguem = 0;
        for(i = 0; i < nos_reais; i++) {
            filhos[i] = 0;
        }

        for(i = 0; i < nos_reais; i++) {
            if (ind->pai[i] != -1) {
                filhos[ind->pai[i]]++;
            }
        }

        for(i = 0; i < nos_reais; i++) {
            if (mapa[i].tipo == 1 && ind->pai[i] != -1 && filhos[i] == 0) {
                ind->pai[i] = -1;
                removeu_alguem = 1;
            }
        }
    } while(removeu_alguem == 1);

    double soma_metros = 0.0;
    for(i = 0; i < nos_reais; i++) {
        if (ind->pai[i] != -1) {
            soma_metros += pit(mapa[i], mapa[ind->pai[i]]);
        }
    }
    ind->distancia_total = soma_metros;
    ind->atenuacao_maxima = 0.0;
}

struct individuo torneio(struct individuo pop_atual[]) {
    int s1 = rand() % TAM_P;
    int s2 = rand() % TAM_P;
    int s3 = rand() % TAM_P;
    struct individuo vencedor = pop_atual[s1];
    if (pop_atual[s2].distancia_total < vencedor.distancia_total) {
        vencedor = pop_atual[s2];
    }
    if (pop_atual[s3].distancia_total < vencedor.distancia_total) {
        vencedor = pop_atual[s3];
    }
    return vencedor;
}

struct individuo roleta(struct individuo pop_atual[]) {
    int i;
    double soma_avaliacoes = 0.0;
    double fatias[TAM_P];

    for (i = 0; i < TAM_P; i++) {
        if (pop_atual[i].distancia_total == 0) {
            fatias[i] = 0.0;
        } else {
            fatias[i] = 1.0 / pop_atual[i].distancia_total;
        }
        soma_avaliacoes += fatias[i];
    }

    double giro = ((double)rand() / RAND_MAX) * soma_avaliacoes;
    double acumulador = 0.0;

    for (i = 0; i < TAM_P; i++) {
        acumulador += fatias[i];
        if (acumulador >= giro) {
            return pop_atual[i];
        }
    }
    return pop_atual[TAM_P - 1];
}

struct individuo cruzamento_arvore(struct individuo *paiA, struct individuo *paiB) {
    struct individuo filho;
    int i;
    int visitado[MAX_NOS];

    for(i = 0; i < nos_reais; i++) {
        filho.pai[i] = -1;
        visitado[i] = 0;
    }

    visitado[0] = 1;
    filho.pai[0] = -1;
    int nos_conectados = 1;

    while (nos_conectados < nos_reais) {
        num_candidatos = 0;

        for (int e = 0; e < total_arestas; e++) {
            int o = lista_arestas[e].u;
            int d = lista_arestas[e].v;
            if (visitado[o] == 1 && visitado[d] == 0 && mapa[o].tipo != 2) {
                if (paiA->pai[d] == o || paiA->pai[o] == d || paiB->pai[d] == o || paiB->pai[o] == d) {
                    cand_origem[num_candidatos] = o;
                    cand_destino[num_candidatos] = d;
                    num_candidatos++;
                }
            }
        }

        if (num_candidatos == 0) {
            for (int e = 0; e < total_arestas; e++) {
                int o = lista_arestas[e].u;
                int d = lista_arestas[e].v;
                if (visitado[o] == 1 && visitado[d] == 0 && mapa[o].tipo != 2) {
                    cand_origem[num_candidatos] = o;
                    cand_destino[num_candidatos] = d;
                    num_candidatos++;
                }
            }
        }
        if (num_candidatos == 0) break;

        int sorteio = rand() % num_candidatos;
        int origem_escolhida = cand_origem[sorteio];
        int destino_escolhido = cand_destino[sorteio];
        filho.pai[destino_escolhido] = origem_escolhida;
        visitado[destino_escolhido] = 1;
        nos_conectados++;
    }
    return filho;
}

int forma_ciclo(int candidato_pai, int no_mutado, int vetor_pais[]) {
    int atual = candidato_pai;
    while (atual != -1) {
        if (atual == no_mutado) {
            return 1;
        }
        atual = vetor_pais[atual];
    }
    return 0;
}

void mutacao_arvore(struct individuo *ind) {
    int chance = rand() % 100;
    if (chance < taxa_mutacao_atual) {
        int no_mutado = (rand() % (nos_reais - 1)) + 1;
        int vizinhos_validos[MAX_NOS];
        int num_vizinhos = 0;
        int i;

        for (i = 0; i < nos_reais; i++) {
            if (matriz_arcos[i][no_mutado] == 1 && ind->pai[i] != -1 && i != ind->pai[no_mutado] && forma_ciclo(i, no_mutado, ind->pai) == 0 && mapa[i].tipo != 2) {
                vizinhos_validos[num_vizinhos] = i;
                num_vizinhos++;
            }
        }

        if (num_vizinhos > 0) {
            int sorteio = rand() % num_vizinhos;
            ind->pai[no_mutado] = vizinhos_validos[sorteio];
        }
    }
}

int main () {
    srand(time(NULL));

    int total_geracoes = 500;
    int repeticoes_por_teste = 3;
    int taxas_teste[] = {10, 30, 50, 70, 90};
    int total_taxas = 5;
    float custo_metro = 1.90;

    // Altere para os nomes reais que estao na sua pasta
    char lista_arquivos[5][50] = {
        "novapalma_oltcidade.txt",
        "interior64.txt",
        "interior64b.txt",
        "cidade64.txt",
        "novapalma128.txt"
    };
    int total_arquivos = 5;

    printf("--- INICIANDO BATERIA DE TESTES FTTX ---\n");
    printf("Configuracao: %d Geracoes | Media de %d Repeticoes\n\n", total_geracoes, repeticoes_por_teste);

    printf("=================================================================================================================\n");
    printf("%-24s | %-10s | %-7s | %-12s | %-12s | %-14s | %-14s\n",
           "Instancia", "Selecao", "Mutacao", "Custo Medio", "Melhor Custo", "Preco Medio", "Melhor Preco");
    printf("=================================================================================================================\n");

    for (int a = 0; a < total_arquivos; a++) {

        FILE *teste_abertura = fopen(lista_arquivos[a], "r");
        if (teste_abertura == NULL) {
            printf("%-24s | ARQUIVO NAO ENCONTRADO\n", lista_arquivos[a]);
            continue;
        }
        fclose(teste_abertura);

        ler_instancia_fttx(lista_arquivos[a]);

        for (int metodo = 1; metodo <= 2; metodo++) {
            char nome_selecao[10];
            if (metodo == 1) strcpy(nome_selecao, "Torneio");
            else strcpy(nome_selecao, "Roleta");

            for (int t = 0; t < total_taxas; t++) {
                taxa_mutacao_atual = taxas_teste[t];

                double soma_distancias_medias = 0.0;
                double recorde_absoluto = 9999999.0;

                for (int rep = 0; rep < repeticoes_por_teste; rep++) {

                    for(int i = 0; i < TAM_P; i++) {
                        gerar_primeira_pop(&pop[i]);
                        avaliar_individuo(&pop[i]);
                    }

                    int geracao = 1;

                    while (geracao <= total_geracoes) {

                        // BARRA DE PROGRESSO VISUAL PARA VOCE SABER QUE NAO TRAVOU
                        if (geracao % 100 == 0) {
                            printf("\rProcessando: %-15s | %-7s | Mut: %d%% | Geracao: %d...       ", lista_arquivos[a], nome_selecao, taxa_mutacao_atual, geracao);
                            fflush(stdout);
                        }

                        int indice_rei = 0;
                        for (int i = 1; i < TAM_P; i++) {
                            if (pop[i].distancia_total < pop[indice_rei].distancia_total) {
                                indice_rei = i;
                            }
                        }
                        pop_nova[0] = pop[indice_rei];

                        for (int i = 1; i < TAM_P; i++) {
                            struct individuo paiA, paiB;

                            if (metodo == 1) {
                                paiA = torneio(pop);
                                paiB = torneio(pop);
                            } else {
                                paiA = roleta(pop);
                                paiB = roleta(pop);
                            }
                            pop_nova[i] = cruzamento_arvore(&paiA, &paiB);
                            mutacao_arvore(&pop_nova[i]);
                            avaliar_individuo(&pop_nova[i]);
                        }

                        for (int i = 0; i < TAM_P; i++) {
                            pop[i] = pop_nova[i];
                        }
                        geracao++;
                    }

                    double melhor_da_rodada = pop[0].distancia_total;
                    soma_distancias_medias += melhor_da_rodada;

                    if (melhor_da_rodada < recorde_absoluto) {
                        recorde_absoluto = melhor_da_rodada;
                    }
                }

                double distancia_media = soma_distancias_medias / repeticoes_por_teste;
                double preco_medio = distancia_media * custo_metro;
                double preco_recorde = recorde_absoluto * custo_metro;

                // Limpa a barra de progresso antes de imprimir a linha final
                printf("\r                                                                                                           \r");
                printf("%-24s | %-10s | %-6d%% | %-9.2f m | %-9.2f m | R$ %-11.2f | R$ %-11.2f\n",
                        lista_arquivos[a], nome_selecao, taxa_mutacao_atual,
                        distancia_media, recorde_absoluto, preco_medio, preco_recorde);
            }
        }
        printf("-----------------------------------------------------------------------------------------------------------------\n");
    }

    printf("\nBateria finalizada com sucesso!\n");
    return 0;
}
