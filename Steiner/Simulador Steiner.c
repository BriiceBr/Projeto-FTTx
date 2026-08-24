#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<math.h>
#include<string.h>

#define TAM_MAPA 52     // Total de pontos do mapa (OLT + postes + clientes)
#define TAM_P 100    // Tamanho da populacao
#define TAX_MUTACAO 40  // Taxa de mutacao

struct poste {
    int id; // numero do poste
    float x; //pos x
    float y; // pos y
    int tipo; // 0 = OLT, 1 = Poste de Passagem, 2 = Cliente
};

struct individuo {
    int pai[TAM_MAPA];       // O vetor que cria a arvore de conexoes
    double distancia_total;  // O gasto total de cabo em metros
    double atenuacao_maxima; // ainda nao usado
};

struct poste mapa[TAM_MAPA];
int matriz_arcos[TAM_MAPA][TAM_MAPA]; // O mapa real das ruas: 1 = tem cabo, 0 = nao tem rua

struct individuo pop[TAM_P];
struct individuo pop_nova[TAM_P];

// Funcao para ler os arq dos arcos e nós
void ler_instancia_fttx(char nome_arq[]) {
    FILE* arq = fopen(nome_arq, "r");
    if (arq == NULL) {
        printf("Erro ao abrir arquivo %s!\n", nome_arq);
        exit(1);
    }
    char linhas[256];
    int lendo_nos = 0;
    int lendo_arcos = 0;

    while (fgets(linhas, sizeof(linhas), arq)) {
        // Identifica quando comeca a ler os nós
        if (strstr(linhas, "Nodes") != NULL) {
            lendo_nos = 1;
            lendo_arcos = 0;
            continue;
        }
        // Desliga a leitura de nos e passa para ler os arcos
        if (strstr(linhas, "Arcs") != NULL) {
            lendo_nos = 0;
            lendo_arcos = 1;
            continue;
        }
        // Desliga a leitura de arcos caso encontre outras secoes
        if (strstr(linhas, "Splitters") != NULL || strstr(linhas, "BalancedSplitters") != NULL) {
            lendo_arcos = 0;
        }

        // LEITURA DOS NÓS
        if (lendo_nos == 1) {
            int id_lido;
            float x, y;

            // Le o ID e depois as coordenadas X e Y
            if (sscanf(linhas, "%d %f %f", &id_lido, &x, &y) == 3) {
                // ID do txt diferente
                int id_correto = id_lido - 1;
                mapa[id_correto].id = id_correto;
                mapa[id_correto].x = x;
                mapa[id_correto].y = y;

                // Classifica o tipo de ponto
                if (id_lido == 1) {
                    mapa[id_correto].tipo = 0; // OLT
                }
                // Adaptando os ultimos 12 pontos como clientes
                else if (id_lido >= (TAM_MAPA - 11) && id_lido <= TAM_MAPA) {
                    mapa[id_correto].tipo = 2; // Cliente Final
                }
                else {
                    mapa[id_correto].tipo = 1; // Poste de Passagem
                }
            }
        }

        // LEITURA DOS ARCOS
        if (lendo_arcos == 1) {
            int origem, destino;
            if (sscanf(linhas, "%d %d", &origem, &destino) == 2) {
                // Preenche a matriz marcando 1 onde existe rua
                matriz_arcos[origem - 1][destino - 1] = 1;
            }
        }
    }

    fclose(arq);
}

// Essa funcao usa o algoritmo de prim randomizado
void gerar_primeira_pop(struct individuo* ind) {
    int i, j;
    int visitado[TAM_MAPA];
    // Limpa o vetor de pais e o vetor de controle
    for(i = 0; i < TAM_MAPA; i++) {
        ind->pai[i] = -1;
        visitado[i] = 0;
    }

    // A raiz da arvore e a OLT (indice 0)
    visitado[0] = 1;
    ind->pai[0] = -1; // OLT nao tem pai

    int nos_conectados = 1;

    // Laco para conectar todos os outros nos do mapa
    while (nos_conectados < TAM_MAPA) {
        // Vetores temporarios para guardar as opcoes de ruas
        int cand_origem[TAM_MAPA * TAM_MAPA]; // nesse ex: 52 x 52 = 2.704
        int cand_destino[TAM_MAPA * TAM_MAPA]; // nesse ex: 52 x 52 = 2.704
        int num_candidatos = 0;

        // Procura todas as ruas validas saindo de quem ja esta na rede para quem ainda nao esta
        for (i = 0; i < TAM_MAPA; i++) {
            if (visitado[i] == 1) {
                for (j = 0; j < TAM_MAPA; j++) {
                    if (matriz_arcos[i][j] == 1 && visitado[j] == 0) {
                        cand_origem[num_candidatos] = i;
                        cand_destino[num_candidatos] = j;
                        num_candidatos++;
                    }
                }
            }
        }
        if (num_candidatos == 0) {
            break;
        }
        // Sorteio de rua
        int sorteio = rand() % num_candidatos;              // No lugar de selecionar a aresta de menor peso, o sistema realiza um rand() % num_candidatos
        int origem_escolhida = cand_origem[sorteio];         //   Isso garante que cada indivíduo fique com uma topologia válida,
        int destino_escolhido = cand_destino[sorteio];      //  conectando todos os pontos, mas com tracados novos.

        // Conecta o novo poste na rede anotando de onde o cabo veio
        ind->pai[destino_escolhido] = origem_escolhida;
        visitado[destino_escolhido] = 1;
        nos_conectados++;
    }
}

// Calculo de distancia com pitagoras igual do outro simulador
double pit(struct poste p1, struct poste p2) {
    float dx = p1.x - p2.x;
    float dy = p1.y - p2.y;
    return sqrt((dx * dx) + (dy * dy));
}

// avaliacao (distancia e atenuacao)
void avaliar_individuo(struct individuo *ind) {
    int i;
    int filhos[TAM_MAPA];
    int removeu_alguem;

    // Repete o corte de cabos ate que nenhum poste inutil sobre
    do {
        removeu_alguem = 0;
        for(i = 0; i < TAM_MAPA; i++) {
            filhos[i] = 0;
        }
        // Conta quantos filhos cada no possui na arvore atual
        for(i = 0; i < TAM_MAPA; i++) {                                                    /// O codigo entende que um poste de passagem (1) que seja um nó folha
            if (ind->pai[i] != -1) {                                                       /// é um desperdicio e corta ele e inclusive o pai se tbm nao tiver ligacao
                filhos[ind->pai[i]]++;
            }
        }
        // Procura postes de passagem (tipo 1) que nao tem filhos
        for(i = 0; i < TAM_MAPA; i++) {
            if (mapa[i].tipo == 1 && ind->pai[i] != -1 && filhos[i] == 0) {
                ind->pai[i] = -1; // corta
                removeu_alguem = 1;
            }
        }
    } while(removeu_alguem == 1);

    // Distancia total do cabeameto
    double soma_metros = 0.0;
    for(i = 0; i < TAM_MAPA; i++) {
        if (ind->pai[i] != -1) {
            soma_metros += pit(mapa[i], mapa[ind->pai[i]]);
        }
    }
    ind->distancia_total = soma_metros;

    // Atenuacao n fiz ainda
    ind->atenuacao_maxima = 0.0;
}

/// Torneio
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

/// Roleta
struct individuo roleta(struct individuo pop_atual[]) {
    int i;
    double soma_avaliacoes = 0.0;
    double fatias[TAM_P];
    // Cria as fatias
    for (i = 0; i < TAM_P; i++) {
        if (pop_atual[i].distancia_total == 0) {
            fatias[i] = 0.0;
        } else {
            fatias[i] = 1.0 / pop_atual[i].distancia_total;
        }
        soma_avaliacoes += fatias[i];
    }
    // Gira a roleta
    double giro = ((double)rand() / RAND_MAX) * soma_avaliacoes;
    double acumulador = 0.0;
    // Verifica onde a bolinha da roleta parou
    for (i = 0; i < TAM_P; i++) {
        acumulador += fatias[i];
        if (acumulador >= giro) {
            return pop_atual[i];
        }
    }
    return pop_atual[TAM_P - 1];
}


/// Cruzamento com base no cruzamento de prim do pdf
struct individuo cruzamento_arvore(struct individuo *paiA, struct individuo *paiB) {
    struct individuo filho;
    int i, j;
    int visitado[TAM_MAPA];
    // limpa
    for(i = 0; i < TAM_MAPA; i++) {
        filho.pai[i] = -1;
        visitado[i] = 0;
    }
    // OLT
    visitado[0] = 1;
    filho.pai[0] = -1;
    int nos_conectados = 1;

    // Monta o filho usando o Prim as ruas dos pais
    while (nos_conectados < TAM_MAPA) {
        int cand_origem[TAM_MAPA * TAM_MAPA];
        int cand_destino[TAM_MAPA * TAM_MAPA];
        int num_candidatos = 0;

        for (i = 0; i < TAM_MAPA; i++) {
            if (visitado[i] == 1) {
                for (j = 0; j < TAM_MAPA; j++) {
                    // A rua existe no mundo fisico E o vizinho ainda nao tem cabo?
                    if (matriz_arcos[i][j] == 1 && visitado[j] == 0) {
                        // O Pai A ou o Pai B passaram por essa rua especifica?
                        if (paiA->pai[j] == i || paiA->pai[i] == j || paiB->pai[j] == i || paiB->pai[i] == j) {
                            cand_origem[num_candidatos] = i;
                            cand_destino[num_candidatos] = j;
                            num_candidatos++;
                        }
                    }
                }
            }
        }

        // Se a construcao travou porque as rotas dos pais nao se encaixaram destrava pegando qualquer rua da cidade
        if (num_candidatos == 0) {
            for (i = 0; i < TAM_MAPA; i++) {
                if (visitado[i] == 1) {
                    for (j = 0; j < TAM_MAPA; j++) {
                        if (matriz_arcos[i][j] == 1 && visitado[j] == 0) {
                            cand_origem[num_candidatos] = i;
                            cand_destino[num_candidatos] = j;
                            num_candidatos++;
                        }
                    }
                }
            }
        }
        if (num_candidatos == 0) break;

        // Sorteia uma das ruas herdadas e conecta no filho
        int sorteio = rand() % num_candidatos;
        int origem_escolhida = cand_origem[sorteio];
        int destino_escolhido = cand_destino[sorteio];
        filho.pai[destino_escolhido] = origem_escolhida;
        visitado[destino_escolhido] = 1;
        nos_conectados++;
    }
    return filho;
}

// Funcao para impedir que a mutacao crie loops isolados
int forma_ciclo(int candidato_pai, int no_mutado, int vetor_pais[]) {
    int atual = candidato_pai;
    // Sobe pela arvore ate achar a OLT (-1)
    while (atual != -1) {
        if (atual == no_mutado) {
            return 1; // ALERTA: Encontrou ele mesmo, vai formar um ciclo!
        }
        atual = vetor_pais[atual];
    }
    return 0; // Caminho livre, pode conectar
}

/// Mutacao
void mutacao_arvore(struct individuo *ind) {
    // Sorteia a chance de a mutacao ocorrer (0 a 99)
    int chance = rand() % 100;
    if (chance < TAX_MUTACAO) {
        // Sorteia um no aleatorio para mutar (ignora a OLT no indice 0)
        int no_mutado = (rand() % (TAM_MAPA - 1)) + 1;
        int vizinhos_validos[TAM_MAPA];
        int num_vizinhos = 0;
        int i;

        // Varre o mapa procurando outras opcoes de ruas para este poste
        for (i = 0; i < TAM_MAPA; i++) {
            //        1. Existe rua?             2. Vizinho ta na rede?    3. Nao e o pai atual?               4. NAO FORMA CICLO?
            if (matriz_arcos[no_mutado][i] == 1 && ind->pai[i] != -1 && i != ind->pai[no_mutado] && forma_ciclo(i, no_mutado, ind->pai) == 0) {
                vizinhos_validos[num_vizinhos] = i;
                num_vizinhos++;
            }
        }
        // Se encontrou uma nova rota viavel, troca o pai e altera a topologia da rede
        if (num_vizinhos > 0) {
            int sorteio = rand() % num_vizinhos;
            ind->pai[no_mutado] = vizinhos_validos[sorteio];
        }
    }
}
main () {
    srand(time(NULL));
    int i, k;
    int total_geracoes;
    int metodo_selecao;
    int pausa_geracao = -1;
    char resp;

    char nome_instancia[50] = "toy3.txt"; // mudar para ooutros arq
    char nome_log[100];
    sprintf(nome_log, "log_%s", nome_instancia);

    ler_instancia_fttx(nome_instancia);

    printf("--- Projeto de Rede FTTx (Arvore de Steiner) ---\n\n");

    printf("Quantas geracoes deseja rodar? ");
    scanf("%d", &total_geracoes);

    printf("Qual metodo de selecao deseja usar? (1 para Torneio, 2 para Roleta): ");
    scanf("%d", &metodo_selecao);

    printf("Deseja pausar em alguma geracao especifica para visualizar? (s/n): ");
    scanf(" %c", &resp); //
    if (resp == 's' || resp == 'S') {
        printf("   Qual geracao? ");
        scanf("%d", &pausa_geracao);
    }
    printf("\nIniciando a evolucao...\n\n");

    // Gera e avalia a populacao inicial
    for(i = 0; i < TAM_P; i++) {
        gerar_primeira_pop(&pop[i]);
        avaliar_individuo(&pop[i]);
    }

    int geracao = 1;
    double melhor_historico = 99999999.0;

    // Evolucao
    while (geracao <= total_geracoes) {

        // Elitismo
        int indice_rei = 0;
        for (i = 1; i < TAM_P; i++) {
            if (pop[i].distancia_total < pop[indice_rei].distancia_total) {
                indice_rei = i;
            }
        }
        pop_nova[0] = pop[indice_rei];

        for (i = 1; i < TAM_P; i++) {
            struct individuo paiA, paiB;

            if (metodo_selecao == 1) {
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

        if (pop_nova[0].distancia_total < melhor_historico) {
            melhor_historico = pop_nova[0].distancia_total;
        }

        // Exibe o progresso
        if (geracao % 100 == 0 || geracao == 1) {
            printf("Geracao %d | Menor Gasto de Cabo: %.2f metros\n", geracao, pop_nova[0].distancia_total);
        }

        // Atualiza a populacao
        for (i = 0; i < TAM_P; i++) {
            pop[i] = pop_nova[i];
        }

        // SISTEMA DE PAUSA E VISUALIZACAO
        if (geracao == pausa_geracao) {
            printf("\n>>> PAUSA NA GERACAO %d <<<\n", geracao);
            int ver_mais = 1;

            while (ver_mais) {
                printf("Deseja visualizar as conexoes de algum individuo desta geracao? (s/n): ");
                scanf(" %c", &resp);

                if (resp == 's' || resp == 'S') {
                    int id_ind;
                    printf("Qual individuo? (0 a %d, onde 0 e o melhor da geracao): ", TAM_P - 1);
                    scanf("%d", &id_ind);

                    if (id_ind >= 0 && id_ind < TAM_P) {
                        printf("\n--- INDIVIDUO %d ---\n", id_ind);
                        printf("Gasto Total de Cabo: %.2f metros\n", pop[id_ind].distancia_total);
                        printf("Mapa de Conexoes:\n");
                        for (k = 0; k < TAM_MAPA; k++) {
                            if (pop[id_ind].pai[k] != -1) {
                                // Soma 1 para a visualizacao bater com as IDs do arquivo de texto
                                printf(" -> Cabo saindo do No %d ate o No %d\n", pop[id_ind].pai[k] + 1, k + 1);
                            }
                        }
                        printf("--------------------\n\n");
                    } else {
                        printf("ID invalido! Tente novamente.\n\n");
                    }
                } else {
                    ver_mais = 0; // Sai do laco de visualizacao
                }
            }

            printf("\nDeseja pausar em outra geracao futura? (s/n): ");
            scanf(" %c", &resp);
            if (resp == 's' || resp == 'S') {
                printf("Qual geracao? ");
                scanf("%d", &pausa_geracao);
                printf("\nRetomando a evolucao...\n\n");
            } else {
                pausa_geracao = -1; // Desliga a pausa pelo resto da execucao
                printf("\nRetomando a evolucao ate o fim...\n\n");
            }
        }
        geracao++;
    }

    printf("\nFim da Evolucao! Melhor rede otimizada encontrou a distancia de: %.2f metros.\n", pop[0].distancia_total);

    /// Log
    FILE *log = fopen(nome_log, "a");
    if (log != NULL) {
        fprintf(log, "========================================\n");
        fprintf(log, "RELATORIO DE EXECUCAO - REDE FTTX\n");
        fprintf(log, "Arquivo analisado: %s\n", nome_instancia);
        fprintf(log, "========================================\n");

        fprintf(log, "CONFIGURACOES DO ALGORITMO:\n");
        fprintf(log, "- Tamanho da Populacao (TAM_P): %d\n", TAM_P);
        fprintf(log, "- Taxa de Mutacao: %d%%\n", TAX_MUTACAO);
        fprintf(log, "- Total de Geracoes: %d\n", total_geracoes);

        if (metodo_selecao == 1) {
            fprintf(log, "- Metodo de Selecao: Torneio\n");
        } else {
            fprintf(log, "- Metodo de Selecao: Roleta\n");
        }

        fprintf(log, "\nRESULTADOS:\n");
        fprintf(log, "- Menor distancia absoluta encontrada: %.2f metros\n", melhor_historico);
        fprintf(log, "- Distancia na geracao final: %.2f metros\n", pop[0].distancia_total);
        fprintf(log, "----------------------------------------\n\n");

        fprintf(log, "\nMAPA DE CONEXOES (ELITE - INDIVIDUO 0):\n");

        // Substitua NUM_NOS pela variavel que representa o total de postes no seu codigo
        for (int i = 0; i < 52; i++) {
            // A condicao abaixo ignora a raiz e os nos desconectados
            // Ajuste o if caso o seu codigo represente a raiz de uma forma diferente (como 0 ou -1)
            if (pop[0].pai[i] != -1 && pop[0].pai[i] != i && pop[0].pai[i] != 0) {
                fprintf(log, "-> Cabo saindo do No %d ate o No %d\n", pop[0].pai[i], i);
            }
        }


        fclose(log);
        printf("\n-> Log salvo com sucesso no arquivo '%s'!\n", nome_log);
    } else {
        printf("\n-> Atencao: Nao foi possivel criar o arquivo de log.\n");
    }

    return 0;
}

