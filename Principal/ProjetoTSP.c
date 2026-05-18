#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<math.h>

///Este código, tem como objetivo simular uma rede de fibra optica, usando como exemplo os exercicios apresentados. Foi criado por conta um arquivo TSP de 20 postes ("rede20.tsp") para simular uma rede */


// 20 postes total dentro do arquivo TSP "rede20.tsp"
#define TAM_ROTA 20
// Tamanho padrao da populacao
#define TAM_P 100

// Os postes possuem um ID e a localizacao
struct poste{
    int id;
    float x;
    float y;
};

// Essa struct serve para guardar cada poste
struct poste mapa[TAM_ROTA];

// Aqui um individuo guarda a sequencia de postes que compoem a rota e a distancia total
struct individuo {
    int rota[TAM_ROTA];
    double distancia_total;
};

// Viveiros globais contendo cada individuo com rota e distancia
struct individuo pop[TAM_P];
struct individuo pop_nova[TAM_P];

// Funcao copiada dos exercicios para a leitura do arquivo TSP
void abrir_arquivo(char nome_arq[]) {
    FILE* arquivo = fopen(nome_arq, "r");
    if (arquivo == NULL) {
        printf("Erro ao abrir o arquivo %s!\n", nome_arq);
        exit(1);
    }
    // linhas do arquivo (Tamanho aumentado para 256 para evitar erro de leitura)
    char linhas[256];
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
    for (i = 0; i < TAM_ROTA; i++) {
        //coloca uma rota em sequencia
        ind->rota[i] = i;
    }

    // O laco para no 1 (i > 1) e o sorteio ignora o 0 (+ 1) para travar a OLT
    for(i = TAM_ROTA - 1; i > 1; i--) {
        //busca uma posicao entre 0 e i (Agora ignorando a posicao da OLT)
        int j = (rand() % i) + 1;
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

    // Salva a nota final
    ind->distancia_total = total;
}

struct individuo torneio(struct individuo pop_atual[]) {
    // Sorteia 3 posições aleatórias no viveiro
    int s1 = rand() % TAM_P;
    int s2 = rand() % TAM_P;
    int s3 = rand() % TAM_P;

    struct individuo vencedor = pop_atual[s1];

    // Verifica quem tem a menor distancia para roubar a coroa
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
        // Sorteia posicoes de 1 para frente para nao arrancar a OLT da primeira gaveta
        int p1 = (rand() % (TAM_ROTA - 1)) + 1;
        int p2 = (rand() % (TAM_ROTA - 1)) + 1;

        int temp = filho.rota[p1];
        filho.rota[p1] = filho.rota[p2];
        filho.rota[p2] = temp;
    }

    return filho;
}

int main () {
    srand(time(NULL));

    int i, j;

    abrir_arquivo("rede20.tsp");

    for(i = 0; i < TAM_P; i++) {
        gerar_primeira_pop(&pop[i]);
        calc_fitness(&pop[i]);
    }

    int geracao = 1;
    double melhor_distancia_historica = 999999999.0;

    printf("\n--- Iniciando a Evolucao da Rede (20 postes - Rota Linear) ---\n");


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

        if (geracao % 1000 == 0 || geracao == 1) {
            printf("Geracao %d | Melhor Rota Atual: %.2f metros\n", geracao, pop_nova[0].distancia_total);
        }

        for (i = 0; i < TAM_P; i++) {
            pop[i] = pop_nova[i];
        }

        geracao++;
    }

    printf("\nCaminho da Fibra:\n");
    printf("OLT 1 ");

    // Laco vai ate o penultimo, calculando a distancia exata para o proximo
    for (int k = 0; k < TAM_ROTA - 1; k++) {
        int p_atual = pop_nova[0].rota[k];
        int p_prox = pop_nova[0].rota[k+1];

        // Mede a distancia especifica desse trecho
        double dist = calcular_distancia(mapa[p_atual], mapa[p_prox]);

        // Exibe o proximo poste e quanto gastou para chegar nele (Soma +1 para visualizacao)
        printf("-> %d (Dist: %.2fm) ", p_prox + 1, dist);
    }

    printf("\n\nFim da Rota! (Rede linear finalizada)\n");

    return 0;
}
