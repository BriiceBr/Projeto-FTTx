#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<math.h>

///Este c√≥digo, tem como objetivo simular uma rede de fibra optica, usando como exemplo os exercicios apresentados. Foi criado por conta um arquivo TSP de 20 postes ("rede20.tsp") para simular uma rede */

/// MELHOR AT… AGORA rede20.tsp -> 439.91m - usando roleta

// Max de nos
#define TAM_ROTA 52 // a suzianne comeÁa nos postes 1 e nao 0
// Tamanho padrao da populacao
#define TAM_P 100
// taxa de mutacao
#define TAX_MUTACAO 5


// Os postes possuem um ID e a localizacao
struct poste{
    int id;
    float x;
    float y;
    int tipo; // 0 OLT, 1 POSTE, 2 CLIENTE
};

int matriz_arcos[TAM_ROTA][TAM_ROTA] = {0};

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

// funcao nova para ler arquivos usados no tcc
void ler_instancia_fttx(char nome_arq[]) {
    FILE* arq = fopen(nome_arq, "r");
    if (arq == NULL) {
        printf("Erro ao abrir arquivo %s!\n", nome_arq);
    }
    char linhas[256];
    int lendo_nos  = 0;
    int lendo_arcos = 0;

    while (fgets(linhas, sizeof(linhas), arq)) {
        // identifica quando comeÁa a ler os nos
        if (strstr(linhas, "Nodes") != NULL) {
            lendo_nos = 1;
            lendo_arcos = 0;
            continue;
        }
        // o codigo desliga a leitura de nos e passa para ler os arcos
        if (strstr(linhas, "Arcs") != NULL) {
            lendo_nos = 0;
            lendo_arcos = 1;
            continue;
        }
        // desliga a leitura de arcos
        if (strstr(linhas, "Splitters") != NULL || strstr(linhas, "BalancedSplitters") != NULL) {
            lendo_arcos = 0;
        }

        // para ler os nos e coordenadas junto com id de cada poste/cliente
        if (lendo_nos == 1) {
            int id_lido;
            float x, y;
            // le o ID dps a coordenada
            if (sscanf(linhas, "%d %f %f", &id_lido, &x, &y) == 3) {
                int id_correto = id_lido - 1;
                mapa[id_correto].id = id_correto;
                mapa[id_correto].x = x;
                mapa[id_correto].y = y;

                // com base no ID lido, o codigo diz qual o tipo daquele poste
                if (id_lido == 1) {
                    mapa[id_correto].tipo = 0; // OLT
                }
                else if (id_lido >= 41 && id_lido <= 52) {
                    mapa[id_correto].tipo = 2; // Cliente Final
                }
                else {
                    mapa[id_correto].tipo = 1; // Poste de Passagem
                }
            }
        }
        // le os arcos e coloca 1 nos arcos se for 0 nao existe aquele arco
        if (lendo_arcos == 1) {
            int origem, destino;
            if (sscanf(linhas, "%d %d", &origem, &destino) == 2) {
                matriz_arcos[origem - 1][destino - 1    ] = 1;
            }
        }
    } // while
    fclose(arq);
}

void gerar_primeira_pop(struct individuo* ind) {
    int i, j;
    // para lembrar quais nos ja foram usados. 0 = Nao visitado / 1 = Visitado
    int visitado[TAM_ROTA] = {0};
    // a cada passo deve ser analisado qual o proximo
    int vizinhos_validos[TAM_ROTA];
    int num_vizinhos;

    for(i = 1; i < TAM_ROTA; i++) {
        ind->rota[i] = -1;
    }
    // sempre sai da OLT
    ind->rota[0] = 0; // OLT
    visitado[0] = 1;  // OLT visitada

    int poste_atual = 0;

    for(i = 1; i < TAM_ROTA; i++) {
        num_vizinhos = 0;
        for(j = 0; j < TAM_ROTA; j++) {
            if (matriz_arcos[poste_atual][j] == 1 && visitado[j] == 0) {
                vizinhos_validos[num_vizinhos] = j;
                num_vizinhos++;
            }
        }
        // senao tiver saida encerra
        if(num_vizinhos == 0) {
            break;
        }
    // sorteia o proximo passo da rede
    int prox_passo = rand() % num_vizinhos;
    int prox_poste = vizinhos_validos[prox_passo];

    ind->rota[i] = prox_poste;
    visitado[prox_poste] = 1;
    poste_atual = prox_poste;
    }

}

// Funcao copiada dos exercicios para a leitura do arquivo TSP
/*void abrir_arquivo(char nome_arq[]) {
    FILE* arquivo = fopen(nome_arq, "r");
    if (arquivo == NULL) {
        printf("Erro ao abrir o arquivo %s!\n", nome_arq);
        exit(1);
    }
    // linhas do arquivo (Tamanho aumentado para 256 para evitar erro de leitura)
    char linhas[256];
    // flag para quando come√ßar a ler as coordenadas
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
            // L√™ o ID, a posi√ß√£o X e a posi√ß√£o Y
            sscanf(linhas, "%d %f %f", &mapa[indice].id, &mapa[indice].x, &mapa[indice].y);
            indice++;
        }
    }
    fclose(arquivo);
}
*/

/*funcao para gerar um individuo em sequencia
void gerar_primeira_pop(struct individuo* ind) {
    int i;
    for(i = 0; i < TAM_ROTA; i++) {
        //coloca uma rota em sequencia
        ind->rota[i] = i;
    }

    // O laco para no 1 (i > 1) e o sorteio ignora o 0 (+ 1) para travar a OLT
    for(i = TAM_ROTA - 1; i > 1; i--) {
        //busca uma posicao entre 0 e i, ignorando a OLT indice 0
        int j = (rand() % i) + 1;
        //troca a posicao
        int temp = ind->rota[i];
        ind->rota[i] = ind->rota[j];
        ind->rota[j] = temp;
    }
}*/

double pit(struct poste p1, struct poste p2) {
    float dx = p1.x - p2.x;
    float dy = p1.y - p2.y;
    return sqrt((dx * dx) + (dy * dy));
}

// Percorre a rota inteira do indivÌduo somando os metros de cabo
void calc_distancia(struct individuo *ind) {
    double total = 0.0;
    int i;

    // Soma do poste 0 at√© o pen√∫ltimo
    for(i = 0; i < TAM_ROTA - 1; i++) {
        int id_atual = ind->rota[i];
        int id_prox = ind->rota[i+1];
        if (id_prox == -1) {
        break;
    }
        total += pit(mapa[id_atual], mapa[id_prox]);
    }

    // Salva a nota final
    ind->distancia_total = total;
}


// Torneio
struct individuo torneio(struct individuo pop_atual[]) {
    // Sorteia 3 posi√ß√µes aleat√≥rias no viveiro
    int s1 = rand() % TAM_P;
    int s2 = rand() % TAM_P;
    int s3 = rand() % TAM_P;

    struct individuo vencedor = pop_atual[s1];

    // Verifica quem tem a menor distancia
    if (pop_atual[s2].distancia_total < vencedor.distancia_total) {
        vencedor = pop_atual[s2];
    }
    if (pop_atual[s3].distancia_total < vencedor.distancia_total) {
        vencedor = pop_atual[s3];
    }

    return vencedor;
}

// Roleta
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
            return pop_atual[TAM_P-1];
        }
    }


}

// mutacao deixei como parametro mesmo tendo uma variavel global, se eu quiser mudar
struct individuo reproducao_e_mutacao(struct individuo *paiA, struct individuo *paiB, int mutacao) {
    struct individuo filho;
    int i, j;
    /// REPRODUCAO (Cruzamento por InterseÁ„o)
    // vetor onde sinaliza qual poste ja foi colocado no novo filho
    int visitado[TAM_ROTA] = {0};
    for(i = 0; i < TAM_ROTA; i++) {
        filho.rota[i] = -1;
    }
    // postes em comum
    int em_comum[TAM_ROTA];
    int num_comum = 0;
    // busca pelos 2 iguais
    for (i = 1; i < TAM_ROTA; i++) {
        int poste_A = paiA->rota[i];
        if (poste_A == -1) break;

        for (j = 1; j < TAM_ROTA; j++) {
            int poste_B = paiB->rota[j];
            if (poste_B == -1) break;

            if (poste_A == poste_B) {
                em_comum[num_comum] = poste_A;
                num_comum++;
                break;
            }
        }
    }
    // nova rota
    if (num_comum > 0) {
        // Sorteia um dos pontos de encontro validos
        int sorteio = rand() % num_comum;
        int poste_corte = em_comum[sorteio];

        // Acha onde o corte vai acontecer
        int ind_corte_A = 0;
        int ind_corte_B = 0;

        for (i = 0; i < TAM_ROTA; i++) {
            if (paiA->rota[i] == poste_corte) {
                ind_corte_A = i;
            }
            if (paiB->rota[i] == poste_corte) {
            ind_corte_B = i;
            }
        }
        // Copia a Parte 1 usando o paiA atÈ o corte 1
        int pos_filho = 0;
        for (i = 0; i <= ind_corte_A; i++) {
            filho.rota[pos_filho] = paiA->rota[i];
            visitado[paiA->rota[i]] = 1;
            pos_filho++;
        }

        // Copia a Parte 2 usando o paiB dps do corte
        for (i = ind_corte_B + 1; i < TAM_ROTA; i++) {
            int cand = paiB->rota[i];
            if (cand == -1) break;

            // trava de seguranca para ndar loop
            if (visitado[cand] == 1) break;

            filho.rota[pos_filho] = cand;
            visitado[cand] = 1;
            pos_filho++;
        }

    } else {
        // Se eles n„o se cruzaram em nenhum momento copia o paiA inteiro
        for (i = 0; i < TAM_ROTA; i++) {
            filho.rota[i] = paiA->rota[i];
        }
    }

    /// MUTACAO
    int chance = rand() % 100;
    if (chance < mutacao) {
        int tam_filho = 0;
        for (i = 0; i < TAM_ROTA; i++) {
            if (filho.rota[i] != -1) tam_filho++;
        }
            // faz a mutacao sem tirar a olt
       if (tam_filho > 4) {
            int ponto_corte = (rand() % (tam_filho - 2)) + 1;
            for (i = ponto_corte; i < TAM_ROTA; i++) {
                filho.rota[i] = -1;
            }
        }
    }

    return filho;
}

void imprimir_pop(struct individuo populacao_impressa[]) {
    int i, j;
    printf("\n --- Imprimindo a populacao --- \n");

    for(i = 0; i < TAM_P; i++) {
        printf("Individuo %d: ", i);

        for (j = 0; j < TAM_ROTA; j++){
            if (populacao_impressa[i].rota[j] == -1) {
                break;
            }
            //
            printf("%d, ", populacao_impressa[i].rota[j] + 1);
        }
        printf("\n");
    }
}

int main () {
    srand(time(NULL));

    int i, j, k; // Variavel k declarada aqui em cima para o Code::Blocks nao dar erro

    // Para ler arquivos tsp
    //abrir_arquivo("rede20.tsp");

    // Para ler instancias do tcc
    ler_instancia_fttx("toy3.txt");

    for(i = 0; i < TAM_P; i++) {
        gerar_primeira_pop(&pop[i]);
        calc_distancia(&pop[i]);
    }
    // quero mostrar a primeira populacao
    imprimir_pop(pop);

    int geracao = 1;
    double melhor_distancia_historica = 999999999.0;

    printf("\n--- Iniciando a Evolucao da Rede (20 postes - Rota Linear) ---\n");


    while (geracao <= 100000) {
        // indice rei √© o melhor individuo da geracao; aplicando o elistismo
        int indice_rei = 0;
        for (j = 1; j < TAM_P; j++) {
            if (pop[j].distancia_total < pop[indice_rei].distancia_total) {
                indice_rei = j;
            }
        }

        pop_nova[0] = pop[indice_rei];

        for (i = 1; i < TAM_P; i++) {
            // roleta() ou torneio()
            struct individuo paiA = roleta(pop);
            struct individuo paiB = roleta(pop);

            pop_nova[i] = reproducao_e_mutacao(&paiA, &paiB, TAX_MUTACAO);
            calc_distancia(&pop_nova[i]);
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
    for (k = 0; k < TAM_ROTA - 1; k++) {
        int p_atual = pop_nova[0].rota[k];
        int p_prox = pop_nova[0].rota[k+1];
        if (p_prox == -1) {
            break;
        }

        // Mede a distancia especifica desse trecho
        double dist = pit(mapa[p_atual], mapa[p_prox]);

        // Exibe o proximo poste e quanto gastou para chegar nele (Soma +1 para visualizacao)
        printf("-> %d (Dist: %.2fm) ", p_prox + 1, dist);
    }

    printf("\n\nFim da Rota! (Rede linear finalizada)\n");

    return 0;
}
