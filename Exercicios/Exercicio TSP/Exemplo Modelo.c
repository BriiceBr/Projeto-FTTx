#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define TAM_ROTA 52
#define TAM_P 100

// A estrutura para guardar os dados do mapa (Postes/Cidades)
struct Cidade {
    int id;
    double x;
    double y;
};

// DNA
struct Individuo {
    int rota[TAM_ROTA];     // A ordem em que visita as cidades
    double distancia_total; // A nota quanto MENOR melhor
};

struct Cidade mapa[TAM_ROTA];

// maior parte IA
void carregar_mapa(char *nome_arquivo) {
    FILE *arquivo = fopen(nome_arquivo, "r");
    if (arquivo == NULL) {
        printf("Erro ao abrir o arquivo %s!\n", nome_arquivo);
        exit(1);
    }

    char linha[256];
    int lendo_coordenadas = 0;
    int index = 0;

    while (fgets(linha, sizeof(linha), arquivo)) {
        if (strncmp(linha, "NODE_COORD_SECTION", 18) == 0) {
            lendo_coordenadas = 1;
            continue;
        }
        if (strncmp(linha, "EOF", 3) == 0) break;

        if (lendo_coordenadas && index < TAM_ROTA) {
            // Lê o ID, a posição X e a posição Y
            sscanf(linha, "%d %lf %lf", &mapa[index].id, &mapa[index].x, &mapa[index].y);
            index++;
        }
    }
    fclose(arquivo);
}

double calcular_distancia(struct Cidade c1, struct Cidade c2) {
    double dx = c1.x - c2.x;
    double dy = c1.y - c2.y;
    return sqrt((dx * dx) + (dy * dy));
}

// O Novo Juiz: Calcula o tamanho total de cabo gasto na rota
void calc_fitness(struct Individuo *ind) {
    double total = 0.0;
    for (int i = 0; i < TAM_ROTA - 1; i++) {
        int id_atual = ind->rota[i];
        int id_prox = ind->rota[i+1];
        total += calcular_distancia(mapa[id_atual], mapa[id_prox]);
    }
    // Soma a volta para o poste inicial para fechar o ciclo
    int ultimo = ind->rota[TAM_ROTA - 1];
    int primeiro = ind->rota[0];
    total += calcular_distancia(mapa[ultimo], mapa[primeiro]);

    ind->distancia_total = total;
}

// Gera o DNA inicial com uma rota totalmente aleatória (sem repetir cidades)
void gerar_dna(struct Individuo *ind) {
    for (int i = 0; i < TAM_ROTA; i++) {
        ind->rota[i] = i;
    }

    // Algoritmo de embaralhamento
    for (int i = TAM_ROTA - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = ind->rota[i];
        ind->rota[i] = ind->rota[j];
        ind->rota[j] = temp;
    }
    calc_fitness(ind);
}

// Torneio invertido: Agora quem ganha a luta é o que tem a MENOR rota
struct Individuo torneio(struct Individuo pop_atual[]) {
    int s1 = rand() % TAM_P;
    int s2 = rand() % TAM_P;
    int s3 = rand() % TAM_P;

    struct Individuo vencedor = pop_atual[s1];

    if (pop_atual[s2].distancia_total < vencedor.distancia_total) {
        vencedor = pop_atual[s2];
    }
    if (pop_atual[s3].distancia_total < vencedor.distancia_total) {
        vencedor = pop_atual[s3];
    }
    return vencedor;
}

// O Novo Cruzamento Inteligente: Previne que o filho visite o mesmo poste duas vezes
struct Individuo reproducao_e_mutacao(struct Individuo *paiA, struct Individuo *paiB, int mutacao) {
    struct Individuo filho;
    int cidade_visitada[TAM_ROTA] = {0}; // Zera as anotações de visita

    // 1) O filho herda a primeira metade exata da rota do Pai A
    int metade = TAM_ROTA / 2;
    for (int i = 0; i < metade; i++) {
        filho.rota[i] = paiA->rota[i];
        cidade_visitada[paiA->rota[i]] = 1; // Marca que essa cidade já está na rota
    }

    // 2) Preenche o restante com as cidades do Pai B (apenas as que faltam)
    int pos_filho = metade;
    for (int i = 0; i < TAM_ROTA; i++) {
        int cidade_candidata = paiB->rota[i];
        if (cidade_visitada[cidade_candidata] == 0) {
            filho.rota[pos_filho] = cidade_candidata;
            cidade_visitada[cidade_candidata] = 1;
            pos_filho++;
        }
    }

    // 3) A Nova Mutação (Swap): Troca dois postes de lugar na rota
    int x = rand() % 100;
    if (x < mutacao) {
        int p1 = rand() % TAM_ROTA;
        int p2 = rand() % TAM_ROTA;
        int temp = filho.rota[p1];
        filho.rota[p1] = filho.rota[p2];
        filho.rota[p2] = temp;
    }

    return filho;
}

int main() {
    srand(time(NULL));

    // Passo essencial: Ler o arquivo e colocar as coordenadas em 'mapa'
    carregar_mapa("berlin52.tsp");

    struct Individuo pop_atual[TAM_P];
    struct Individuo pop_nova[TAM_P];

    // Geração 0 (População Inicial)
    for (int i = 0; i < TAM_P; i++) {
        gerar_dna(&pop_atual[i]);
    }

    int mutacao = 10; // Para o TSP, mutações mais altas ajudam a não estagnar
    double melhor_distancia_geral = 99999999.0;
    int Numero_Geracao = 1;

    printf("\n--- Iniciando Evolucao do Caixeiro Viajante (TSP) ---\n");

    // Condição de parada: Rodar por 1000 gerações para ver o algoritmo convergir
    while (Numero_Geracao <= 1000) {
        struct Individuo paiA, paiB;
        double melhor_dessa_geracao = 99999999.0;

        for (int i = 0; i < TAM_P; i++) {
            paiA = torneio(pop_atual);
            paiB = torneio(pop_atual);

            pop_nova[i] = reproducao_e_mutacao(&paiA, &paiB, mutacao);
            calc_fitness(&pop_nova[i]);

            if (pop_nova[i].distancia_total < melhor_dessa_geracao) {
                melhor_dessa_geracao = pop_nova[i].distancia_total;
            }

            if (pop_nova[i].distancia_total < melhor_distancia_geral) {
                melhor_distancia_geral = pop_nova[i].distancia_total;
            }
        }

        // Imprime o progresso a cada 100 gerações para não floodar a tela
        if (Numero_Geracao % 100 == 0 || Numero_Geracao == 1) {
            printf("Geracao %d | Menor Rota Encontrada: %.2f\n", Numero_Geracao, melhor_dessa_geracao);
        }

        // A Grande Troca
        for (int i = 0; i < TAM_P; i++) {
            pop_atual[i] = pop_nova[i];
        }

        Numero_Geracao++;
    }

    printf("\n--- Evolucao Concluida! ---\n");
    printf("A rota mais economica encontrada tem %.2f de comprimento.\n", melhor_distancia_geral);

    return 0;
}
