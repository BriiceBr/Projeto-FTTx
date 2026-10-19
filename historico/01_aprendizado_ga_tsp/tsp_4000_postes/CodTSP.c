#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<math.h>

/* Esse codigo tem como o objetivo ser simples mas funcional para criar as redes TSP, baseado no exercicio modelo e no exercicio de AG com Hello World */


// 4461 postes total dentro do arquivo TSP
#define TAM_ROTA 4461
//Tamanho padrao da populacao
#define TAM_P 100

// os postes possuem um ID e a localizacao
struct poste{
    int id;
    float x;
    float y;

};

// essa struct serve para guardar cada poste
struct poste mapa[TAM_ROTA];

// aqui um individuo guarda a sequencia de postes que compoem a rota e a distancia total
struct individuo {
    int rota[TAM_ROTA];
    double distancia_total;

};

// viveiros globais
struct individuo pop[TAM_P];
struct individuo pop_nova[TAM_P];

// funcao pega do modelo
void abrir_arquivo(char nome_arq[]) {
    FILE* arquivo = fopen(nome_arq, "r");
    if (arquivo == NULL) {
        printf("Erro ao abrir o arquivo %s!\n", nome_arq);
        exit(1);
    }
    // linhas do arquivo
    char linhas[4467];
    // flag para quando começar a ler as coordenadas
    int lendo_coordenadas = 0;
    // este indice indica qual o poste que vai receber as coordenadas e o ID
    int indice = 0;

      while (fgets(linhas, sizeof(linhas), arquivo)) {
        if (strncmp(linhas, "NODE_COORD_SECTION", 18) == 0) {
            lendo_coordenadas = 1;
            continue;
        }
        if (strncmp(linhas, "EOF", 3) == 0) break;

        if (lendo_coordenadas && indice < TAM_ROTA) {
            // Lê o ID, a posição X e a posição Y
            sscanf(linhas, "%d %f %f", &mapa[indice].id, &mapa[indice].x, &mapa[indice].y);
            indice++;
        }
    }
    fclose(arquivo);
}

//funcao para gerar um individuo em sequencia
void gerar_primeira_pop(struct individuo* ind) {
    int i;
    for (i = 0; i < 4461; i++) {
        //coloca uma rota em sequencia
        ind->rota[i] = i;
    }
    for(i = TAM_ROTA - 1; i > 0; i--) {
        //busca uma posicao entre 0 e i
        int j = rand() % (i+1);
        //troca a posicao
        int temp = ind->rota[i];
        ind->rota[i] = ind->rota[j];
        ind->rota[j] = temp;
    }
}

double calcular_distancia(struct poste p1, struct poste p2) {
    float dx = p1.x - p2.x;
    float dy = p1.y - p2.y;
    return sqrt((dx * dx) + (dy * dy));
}

// Percorre a rota inteira do indivíduo somando os metros de cabo
void calc_fitness(struct individuo *ind) {
    double total = 0.0;
    int i;

    // Soma do poste 0 até o penúltimo
    for (i = 0; i < TAM_ROTA - 1; i++) {
        int id_atual = ind->rota[i];
        int id_prox = ind->rota[i+1];
        total += calcular_distancia(mapa[id_atual], mapa[id_prox]);
    }

    // Soma a volta do último poste de volta para o primeiro
    int id_ultimo = ind->rota[TAM_ROTA - 1];
    int id_primeiro = ind->rota[0];
    total += calcular_distancia(mapa[id_ultimo], mapa[id_primeiro]);

    // Salva a nota final
    ind->distancia_total = total;
}

struct individuo torneio(struct individuo pop_atual[]) {
    // Sorteia 3 posições aleatórias no viveiro
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

struct individuo reproducao_e_mutacao(struct individuo *paiA, struct individuo *paiB, int mutacao) {
    struct individuo filho;

    int cidade_visitada[TAM_ROTA] = {0};
    int i;


    int metade = TAM_ROTA / 2;
    for (i = 0; i < metade; i++) {
        filho.rota[i] = paiA->rota[i];
        cidade_visitada[paiA->rota[i]] = 1;
    }

    int pos_filho = metade;
    for (i = 0; i < TAM_ROTA; i++) {
        int cidade_candidata = paiB->rota[i];


        if (cidade_visitada[cidade_candidata] == 0) {
            filho.rota[pos_filho] = cidade_candidata;
            cidade_visitada[cidade_candidata] = 1;
            pos_filho++;
        }
    }

    int chance = rand() % 100;
    if (chance < mutacao) {
        int p1 = rand() % TAM_ROTA;
        int p2 = rand() % TAM_ROTA;

        int temp = filho.rota[p1];
        filho.rota[p1] = filho.rota[p2];
        filho.rota[p2] = temp;
    }

    return filho;
}

int main () {
    srand(time(NULL));

    int i, j;

    abrir_arquivo("fnl4461.tsp");

    for(i = 0; i < TAM_P; i++) {
        gerar_primeira_pop(&pop[i]);
        calc_fitness(&pop[i]);
    }

    int geracao = 1;
    double melhor_distancia_historica = 999999999.0;

    printf("\n--- Iniciando a Evolucao da Rede (4461 postes) ---\n");


    while (geracao <= 10000) {
        int indice_rei = 0;
        for (j = 1; j < TAM_P; j++) {
            if (pop[j].distancia_total < pop[indice_rei].distancia_total) {
                indice_rei = j;
            }
        }

        pop_nova[0] = pop[indice_rei];

        for (i = 1; i < TAM_P; i++) {
            struct individuo paiA = torneio(pop);
            struct individuo paiB = torneio(pop);

            pop_nova[i] = reproducao_e_mutacao(&paiA, &paiB, 10);
            calc_fitness(&pop_nova[i]);
        }

        if (pop_nova[0].distancia_total < melhor_distancia_historica) {
            melhor_distancia_historica = pop_nova[0].distancia_total;
        }

        if (geracao % 10 == 0 || geracao == 1) {
            printf("Geracao %d | Melhor Rota Atual: %.2f metros\n", geracao, pop_nova[0].distancia_total);
        }

        for (i = 0; i < TAM_P; i++) {
            pop[i] = pop_nova[i];
        }

        geracao++;
    }

    return 0;
}
