    #include<stdio.h>
    #include<stdlib.h>
    #include<time.h>
    #include<math.h>
    #include<string.h>

    #define RAIO_TERRA 6371000.0
    #define PI 3.14159265358979323846

    #define TAM_MAPA 175   // Total de pontos do mapa (OLT + postes + clientes)
    #define TAM_P 100   // Tamanho da populacao
    #define TAX_MUTACAO 1  // Taxa de mutacao

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

    // Variavel global para armazenar a escolha (1 = Pitagoras, 2 = Alternativa)
int tipo_calculo;

// Funcao auxiliar para converter graus GPS em radianos
double para_radianos(double graus) {
    return graus * PI / 180.0;
}

// 1. Calculo plano para a instancia Sintetica
double calculo_pitagoras(struct poste p1, struct poste p2) {
    float dx = p1.x - p2.x;
    float dy = p1.y - p2.y;
    return sqrt((dx * dx) + (dy * dy));
}

// 2. Calculo real para Nova Palma (Fórmula de Haversine)
double calculo_coordenadas_reais(struct poste p1, struct poste p2) {
    // Assume que mapa.x = Latitude e mapa.y = Longitude
    double lat1 = para_radianos(p1.x);
    double lon1 = para_radianos(p1.y);
    double lat2 = para_radianos(p2.x);
    double lon2 = para_radianos(p2.y);

    double dlat = lat2 - lat1;
    double dlon = lon2 - lon1;

    double a = sin(dlat / 2.0) * sin(dlat / 2.0) +
               cos(lat1) * cos(lat2) *
               sin(dlon / 2.0) * sin(dlon / 2.0);

    double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));

    return RAIO_TERRA * c; // Retorna a metragem exata de cabo
}

// 3. A funcao "coringa" que decide qual matematica usar com base no menu
double calcular_distancia(struct poste p1, struct poste p2) {
    if (tipo_calculo == 1) {
        return calculo_pitagoras(p1, p2);
    } else {
        return calculo_coordenadas_reais(p1, p2);
    }
}


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
        int num_clientes = 0;

        while (fgets(linhas, sizeof(linhas), arq)) {
            // Identifica a quantidade de clientes no cabecalho do txt
            if (strstr(linhas, "Clients") != NULL) {
                sscanf(linhas, "Clients %d", &num_clientes);
                continue;
            }

            // Identifica quando comeca a ler os nós
            if (strstr(linhas, "Nodes") != NULL) {
                lendo_nos = 1;
                lendo_arcos = 0;
                continue;
            }
            // Desliga a leitura de nos e passa para ler os arcos
            if (strstr(linhas, "Arc") != NULL) {
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
                    // Automatizado: os ultimos 'num_clientes' da rede sao clientes
                    else if (num_clientes > 0 && id_lido > (TAM_MAPA - num_clientes)) {
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
           // Vetores temporarios para guardar as opcoes de ruas e suas distancias
            int cand_origem[TAM_MAPA * 10];
            int cand_destino[TAM_MAPA * 10];
            double cand_dist[TAM_MAPA * 10]; // Novo vetor para guardar a distancia
                int num_candidatos = 0;
            int c1, c2; // Variaveis para a ordenacao

            // Procura todas as ruas validas saindo de quem ja esta na rede para quem ainda nao esta
            for (i = 0; i < TAM_MAPA; i++) {
                if (visitado[i] == 1) {
                    for (j = 0; j < TAM_MAPA; j++) {
                        if (matriz_arcos[i][j] == 1 && visitado[j] == 0 && mapa[i].tipo != 2) {
                            cand_origem[num_candidatos] = i;
                            cand_destino[num_candidatos] = j;
                            // Calcula a distancia dessa rua e guarda no vetor
                            cand_dist[num_candidatos] = calcular_distancia(mapa[i], mapa[j]);
                            num_candidatos++;
                        }
                    }
                }
            }

            if (num_candidatos == 0) {
                break;
            }

            // Ordena os candidatos da rua mais curta para a rua mais longa
            for (c1 = 0; c1 < num_candidatos - 1; c1++) {
                for (c2 = c1 + 1; c2 < num_candidatos; c2++) {
                    if (cand_dist[c2] < cand_dist[c1]) {
                        // Inverte as distancias
                        double temp_d = cand_dist[c1];
                        cand_dist[c1] = cand_dist[c2];
                        cand_dist[c2] = temp_d;
                        // Inverte a origem
                        int temp_o = cand_origem[c1];
                        cand_origem[c1] = cand_origem[c2];
                        cand_origem[c2] = temp_o;
                        // Inverte o destino
                        int temp_dest = cand_destino[c1];
                        cand_destino[c1] = cand_destino[c2];
                        cand_destino[c2] = temp_dest;
                    }
                }
            }

            // Restringe o sorteio para ocorrer APENAS entre os X melhores caminhos
            int limite_sorteio = num_candidatos;
            if (limite_sorteio > 5) {
                limite_sorteio = 5;
            }

            int sorteio = rand() % limite_sorteio;

            int origem_escolhida = cand_origem[sorteio];
            int destino_escolhido = cand_destino[sorteio];

            // Conecta o novo poste na rede anotando de onde o cabo veio
            ind->pai[destino_escolhido] = origem_escolhida;
            visitado[destino_escolhido] = 1;
            nos_conectados++;
        }
    }

    // avaliacao (distancia e atenuacao)
    void avaliar_individuo(struct individuo *ind) {
        int i;
        int filhos[TAM_MAPA];
        int removeu_alguem;

        // Repete o corte de cabos ate que nenhum poste inutil sobre
        do {
            removeu_alguem = 0;
            for(i = 0; i < TAM_MAPA; i++) { // zera os filhos
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
                soma_metros += calcular_distancia(mapa[i], mapa[ind->pai[i]]);
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


 /// Cruzamento Guiado (Agora enxerga as distancias)
    struct individuo cruzamento_arvore(struct individuo *paiA, struct individuo *paiB) {
        struct individuo filho;
        int i, j, c1, c2;
        int visitado[TAM_MAPA];
        for(i = 0; i < TAM_MAPA; i++) {
            filho.pai[i] = -1;
            visitado[i] = 0;
        }
        visitado[0] = 1;
        filho.pai[0] = -1;
        int nos_conectados = 1;

        while (nos_conectados < TAM_MAPA) {
            int cand_origem[TAM_MAPA * 10];
            int cand_destino[TAM_MAPA * 10];
            double cand_dist[TAM_MAPA * 10]; // Nova memoria para distancias
            int num_candidatos = 0;

            // Tenta usar as ruas dos pais primeiro
            for (i = 0; i < TAM_MAPA; i++) {
                if (visitado[i] == 1) {
                    for (j = 0; j < TAM_MAPA; j++) {
                        if (matriz_arcos[i][j] == 1 && visitado[j] == 0 && mapa[i].tipo != 2) {
                            if (paiA->pai[j] == i || paiA->pai[i] == j || paiB->pai[j] == i || paiB->pai[i] == j) {
                                cand_origem[num_candidatos] = i;
                                cand_destino[num_candidatos] = j;
                                cand_dist[num_candidatos] = calcular_distancia(mapa[i], mapa[j]);
                                num_candidatos++;
                            }
                        }
                    }
                }
            }

            // Fallback: Se os pais nao deram opcao, olha o mapa todo
            if (num_candidatos == 0) {
                for (i = 0; i < TAM_MAPA; i++) {
                    if (visitado[i] == 1) {
                        for (j = 0; j < TAM_MAPA; j++) {
                            if (matriz_arcos[i][j] == 1 && visitado[j] == 0 && mapa[i].tipo != 2) {
                                cand_origem[num_candidatos] = i;
                                cand_destino[num_candidatos] = j;
                                cand_dist[num_candidatos] = calcular_distancia(mapa[i], mapa[j]);
                                num_candidatos++;
                            }
                        }
                    }
                }
            }
            if (num_candidatos == 0) break;

            // ORDENACAO GULOSA: Organiza as opcoes da mais curta para a mais longa
            for (c1 = 0; c1 < num_candidatos - 1; c1++) {
                for (c2 = c1 + 1; c2 < num_candidatos; c2++) {
                    if (cand_dist[c2] < cand_dist[c1]) {
                        double temp_d = cand_dist[c1];
                        cand_dist[c1] = cand_dist[c2];
                        cand_dist[c2] = temp_d;
                        int temp_o = cand_origem[c1];
                        cand_origem[c1] = cand_origem[c2];
                        cand_origem[c2] = temp_o;
                        int temp_dest = cand_destino[c1];
                        cand_destino[c1] = cand_destino[c2];
                        cand_destino[c2] = temp_dest;
                    }
                }
            }

            // Sorteia apenas entre as 3 melhores opcoes (Top-3)
            int limite_sorteio = num_candidatos;
            if (limite_sorteio > 3) {
                limite_sorteio = 3;
            }
            int sorteio = rand() % limite_sorteio;

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

/// Mutacao Guiada (Agora enxerga as distancias)
    void mutacao_arvore(struct individuo *ind) {
        int i, c1, c2;
        for (int no_mutado = 1; no_mutado < TAM_MAPA; no_mutado++) {
            int chance = rand() % 100;
            if (chance < TAX_MUTACAO) {
                int vizinhos_validos[TAM_MAPA];
                double dist_vizinhos[TAM_MAPA]; // Memoriza a distancia
                int num_vizinhos = 0;

                for (i = 0; i < TAM_MAPA; i++) {
                    if (matriz_arcos[i][no_mutado] == 1 && ind->pai[i] != -1 && i != ind->pai[no_mutado] && forma_ciclo(i, no_mutado, ind->pai) == 0 && mapa[i].tipo != 2) {
                        vizinhos_validos[num_vizinhos] = i;
                        dist_vizinhos[num_vizinhos] = calcular_distancia(mapa[i], mapa[no_mutado]);
                        num_vizinhos++;
                    }
                }

                if (num_vizinhos > 0) {
                    // ORDENACAO GULOSA: Organiza da rua mais curta para a mais longa
                    for (c1 = 0; c1 < num_vizinhos - 1; c1++) {
                        for (c2 = c1 + 1; c2 < num_vizinhos; c2++) {
                            if (dist_vizinhos[c2] < dist_vizinhos[c1]) {
                                double temp_d = dist_vizinhos[c1];
                                dist_vizinhos[c1] = dist_vizinhos[c2];
                                dist_vizinhos[c2] = temp_d;
                                int temp_v = vizinhos_validos[c1];
                                vizinhos_validos[c1] = vizinhos_validos[c2];
                                vizinhos_validos[c2] = temp_v;
                            }
                        }
                    }

                    // Sorteia apenas entre as 3 melhores ruas disponiveis
                    int limite = num_vizinhos;
                    if(limite > 3) limite = 3;
                    int sorteio = rand() % limite;

                    ind->pai[no_mutado] = vizinhos_validos[sorteio];
                }
            }
        }
    }

int main () {
        srand(time(NULL));
        int i, k;
        int total_geracoes;
        int metodo_selecao;

        char nome_instancia[50] = "interior64b.txt";
        char nome_log[100];
        sprintf(nome_log, "log_%s", nome_instancia);
        printf("Instancia: %s\n", nome_instancia);

        ler_instancia_fttx(nome_instancia);

        printf("--- Projeto de Rede FTTx (Arvore de Steiner) ---\n\n");

        printf("Quantas geracoes deseja rodar? ");
        scanf("%d", &total_geracoes);

        printf("Qual metodo de selecao deseja usar? (1 para Torneio, 2 para Roleta): ");
        scanf("%d", &metodo_selecao);

        printf("Qual formula de distancia deseja aplicar? (1 para Pitagoras, 2 para Alternativa): ");
        scanf("%d", &tipo_calculo);

        printf("\nIniciando a evolucao...\n\n");

        // MARCADOR DE INICIO DO TEMPO
        clock_t inicio_tempo = clock();

        // Gera e avalia a populacao inicial
        for(i = 0; i < TAM_P; i++) {
            gerar_primeira_pop(&pop[i]);
            avaliar_individuo(&pop[i]);
        }

        int geracao = 1;
        double melhor_historico = 99999999.0;
        int geracoes_sem_melhora = 0;

        // Evolucao
        while (geracao <= total_geracoes) {

            // Elitismo Restaurado
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

            // ATUALIZACAO DO CRITERIO DE PARAGEM
            if (pop_nova[0].distancia_total < melhor_historico) {
                melhor_historico = pop_nova[0].distancia_total;
                geracoes_sem_melhora = 0;
            } else {
                geracoes_sem_melhora++;
            }

            // GATILHO DE PARAGEM
            if (geracoes_sem_melhora >= 100000) {
                printf("\n>>> Paragem antecipada na geracao %d! Algoritmo estagnou por 50 geracoes no limite de %.2f metros.\n", geracao, melhor_historico);
                break;
            }

            // Exibe o progresso
            if (geracao % 100 == 0 || geracao == 1) {
                printf("Geracao %d | Menor Gasto de Cabo: %.2f metros\n", geracao, pop_nova[0].distancia_total);
            }

            // Atualiza a populacao
            for (i = 0; i < TAM_P; i++) {
                pop[i] = pop_nova[i];
            }

            geracao++;
        }

        // MARCADOR DE FIM DO TEMPO (Imediatamente apos o fim das iteracoes)
        clock_t fim_tempo = clock();
        double tempo_execucao = (double)(fim_tempo - inicio_tempo) / CLOCKS_PER_SEC;

        printf("\nFim da Evolucao! Melhor rede otimizada encontrou a distancia de: %.2f metros.\n", pop[0].distancia_total);
        printf("Tempo computacional de execucao: %f segundos.\n\n", tempo_execucao);

        // IMPRESSAO OBRIGATORIA DO MAPA FINAL
        printf("--- MAPA DE CONEXOES (MELHOR INDIVIDUO) ---\n");
        for (k = 0; k < TAM_MAPA; k++) {
            if (pop[0].pai[k] != -1 && pop[0].pai[k] != k) {
                printf(" -> Cabo saindo do No %d ate o No %d\n", pop[0].pai[k] + 1, k + 1);
            }
        }
        printf("-------------------------------------------\n\n");

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
            fprintf(log, "- Tempo computacional: %f segundos\n", tempo_execucao);
            fprintf(log, "----------------------------------------\n\n");
            fprintf(log, "\nMAPA DE CONEXOES (ELITE - INDIVIDUO 0):\n");

            for (int j = 0; j < TAM_MAPA; j++) {
                if (pop[0].pai[j] != -1 && pop[0].pai[j] != j) {
                    fprintf(log, "-> Cabo saindo do No %d ate o No %d\n", pop[0].pai[j] + 1, j + 1);
                }
            }

            fclose(log);
            printf("-> Log salvo com sucesso no arquivo '%s'!\n", nome_log);
        } else {
            printf("-> Atencao: Nao foi possivel criar o arquivo de log.\n");
        }

        return 0;
    }

