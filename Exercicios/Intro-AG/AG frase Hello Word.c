#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>


#define TAM_FRASE 11
#define TAM_P 100
struct ind{
    char dna[12]; // frase "Hello World" + '\0'
    int fitness;          // Nota de 0 a 11
};


void gerar_dna(char *alfabeto, struct ind *individuo) {
    int rLetra;
    for (int i = 0; i < TAM_FRASE; i++) {
        rLetra = rand() % 27; // 26 letras + 1 espaço = 27 chars
        individuo->dna[i] = alfabeto[rLetra];
    }
    individuo->dna[TAM_FRASE] = '\0';
    individuo->fitness = 0;
}

void calc_fitness(char *str, struct ind* ind) {
    ind->fitness = 0;
    for (int i = 0; i < TAM_FRASE; i++) {
        if (ind->dna[i] == str[i]) {
            ind->fitness += 1;
        }
    }
}

struct ind torneio(struct ind pop_atual[]) {
    int sorteado1 = rand() % 100;
    int sorteado2 = rand() % 100;
    int sorteado3 = rand() % 100;

    struct ind ind1 = pop_atual[sorteado1];
    struct ind ind2 = pop_atual[sorteado2];
    struct ind ind3 = pop_atual[sorteado3];

    struct ind vencedor = ind1;

    if (ind2.fitness > vencedor.fitness) {
        vencedor = ind2;
    }
    if (ind3.fitness > vencedor.fitness) {
        vencedor = ind3;
    }
   // printf("\nFitness Do Vencedor: %d\n", vencedor.fitness);
    return vencedor;
}

struct ind reproducao_e_mutacao(struct ind *PaiA, struct ind *PaiB, char *alfabeto, int mutacao) {
    int i;
    struct ind filho = *PaiA;
    for (i = 6; i < TAM_FRASE+1; i++) { // copia ate a metade e trocar com o outro pai ate o final
        filho.dna[i] = PaiB->dna[i];
    }
    // printf("\n%s\n", filho.dna);
    int x = rand() % 100;               // x = representa chance de mutacao
    if (x <= mutacao-1) {
        x = rand() % 27;                // x = agora se torna uma letra do alfabeto
        int y = rand() % TAM_FRASE;     // y = qual sera o segmento do dna que sofrera a mutacao
        filho.dna[y] = alfabeto[x];
    }

    return filho;
}

main () {
    int p = 0;
    char str[] = "HELLO WORLD"; // DETALHE: a string deve ser em letras minusculas pois senao, letras maiusculas tambem deveriam estar no alfabeto
    char ALFABETO[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ "; // 26 letras + 1 espaço
    srand(time(NULL));

    // Objetivo
    printf("Objetivo: %s\n", str);
    printf("Gerando uma população inicial de %d individuos... \n", TAM_P);

    //  Onde moram os individuos da geração ATUAL! A grosso modo: um viveiro
    struct ind pop_atual[TAM_P];
    //  Onde moram os individuos da geração NOVA!
    struct ind pop_nova[TAM_P];

    // Criando a população e as notas de cada ind...
    int i = 0;
    while (i < TAM_P) {
        gerar_dna(&ALFABETO, &pop_atual[i]); // desta forma cada indice do vetor pop_atual É um INDIVIDUO.
        calc_fitness(&str, &pop_atual[i]);
        i++;
    }

    printf("\n ---Populacao inicial !!! ---\n");
    for (i=0; i < TAM_P; i++) {
        printf("Individuo %d: %s... Fitness: %d\n", i+1, pop_atual[i].dna, pop_atual[i].fitness);
    }

    // USER DEFINE A TAXA DE MUTACAO
    int mutacao = 0;
    printf("\nDigite a chance de mutacao com valores inteiros (Exemplo: 5) : \n");
    scanf("%d", &mutacao);

    // CRIANDO O CLICO DE VIDA DO CODIGO
    int melhor_fitness = 0;
    int Numero_Geracao = 1;
    int indice_melhor = 0;


    printf("\n --- Iniciando Evolucao Do Objetivo ! ---\n");

    while (melhor_fitness < TAM_FRASE) {

        struct ind paiA, paiB;
        int fitness_geracao = 0;

        // 1) Inicia a geracao nova
        for (i = 0; i < TAM_P; i++) {
            // 1.1) Escolhe os pais com melhor fitness
            paiA = torneio(&pop_atual);
            paiB = torneio(&pop_atual);
            // 1.2) Copia o filho para a nova populacao
            pop_nova[i] = reproducao_e_mutacao(&paiA, &paiB, &ALFABETO, mutacao);
            // 1.3) Calcula o fitness de cada um dele
            calc_fitness(str, &pop_nova[i]);
            // 1.4) Verifica se esse filho eh o maior fitness
            if (pop_nova[i].fitness > melhor_fitness) {
                melhor_fitness = pop_nova[i].fitness;
            }
            // 1.5) O melhor dessa rodada
            if (pop_nova[i].fitness > fitness_geracao) {
                fitness_geracao = pop_nova[i].fitness;
                indice_melhor = i;
            }
        }
        // 2) Termina a geracao nova e mostra na tela
        printf("\n ---Geracao Nova !!! ---\n");
        for (i=0; i < TAM_P; i++) {
            printf("Geracao %d | Melhor Fitness: %d | Individuo %d | Fitness: %d | DNA: %s\n", Numero_Geracao, melhor_fitness, i+1, pop_nova[i].fitness, pop_nova[i].dna);
        }

        // 3) Geracao nova se torna a atual
        for (i = 0; i < TAM_P; i++) {
            pop_atual[i] = pop_nova[i];
        }
        // 4) Finaliza a geracao
        Numero_Geracao++;
    }

    // ACABA O CODIGO
    printf("\n--- SUCESSO! O Algoritmo chegou em '%s' na Geracao %d, individuo %d! ---\n", str, Numero_Geracao - 1, indice_melhor+1);
    return 0;
}

