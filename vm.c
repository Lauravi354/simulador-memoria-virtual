#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define TAM_PAGINA 256
#define TAM_ESPACO_VIRTUAL (256L * TAM_PAGINA)
#define TOTAL_PAGINAS 256
#define TOTAL_FRAMES 128
#define TOTAL_ENTRADAS_TLB 16

typedef struct {
    int pagina;
    int frame;
    int ordem;
} EntradaTLB;

typedef struct {
    int pagina_procurada;
    int pagina_da_entrada;
    int encontrou;
} DadosBuscaTLB;

void *buscar_entrada_tlb(void *argumento)
{
    DadosBuscaTLB *dados = argumento;

    dados->encontrou = dados->pagina_procurada == dados->pagina_da_entrada;

    return NULL;
}

int main(int argc, char *argv[]){
    if (argc != 4) {
        printf("Uso: %s address.txt fifo|lru fifo|lru\n", argv[0]);
        return 1;
    }

    int usar_fifo = strcmp(argv[2], "fifo") == 0;
    int usar_lru = strcmp(argv[2], "lru") == 0;

    if (usar_fifo == 0 && usar_lru == 0) {
        printf("Algoritmo da RAM invalido. Use fifo ou lru.\n");
        return 1;
    }

    int usar_fifo_tlb = strcmp(argv[3], "fifo") == 0;
    int usar_lru_tlb = strcmp(argv[3], "lru") == 0;

    if (usar_fifo_tlb == 0 && usar_lru_tlb == 0) {
        printf("Algoritmo da TLB invalido. Use fifo ou lru.\n");
        return 1;
    }

    FILE *arquivo = fopen(argv[1], "r");
    if (arquivo == NULL) {
        printf("Nao foi possivel abrir o arquivo %s.\n", argv[1]);
        return 1;
    }

    FILE *disco = fopen("BACKING_STORE.bin", "rb");
    if (disco == NULL) {
        printf("Nao foi possivel abrir BACKING_STORE.bin.\n");
        fclose(arquivo);
        return 1;
    }

    char armazenamento[TAM_ESPACO_VIRTUAL];

    for (int i = 0; i < TAM_ESPACO_VIRTUAL; i++) {
        int byte = fgetc(disco);

        if (byte == EOF) {
            printf("Nao foi possivel ler todo o BACKING_STORE.bin.\n");
            fclose(disco);
            fclose(arquivo);
            return 1;
        }

        if (CHAR_MIN < 0 && byte >= 128) {
            byte = byte - 256;
        }

        armazenamento[i] = byte;
    }

    fclose(disco);

    int tabela_paginas[TOTAL_PAGINAS];
    char memoria[TOTAL_FRAMES][TAM_PAGINA];
    int proximo_frame = 0;
    int pagina_do_frame[TOTAL_FRAMES];
    int proxima_vitima_fifo = 0;
    int ultimo_acesso[TOTAL_FRAMES];

    for (int i = 0; i < TOTAL_PAGINAS; i++) {
        tabela_paginas[i] = -1;
    }

    char linha[128];
    int numero_linha = 0;

    EntradaTLB tlb[TOTAL_ENTRADAS_TLB];

    for (int i = 0; i < TOTAL_ENTRADAS_TLB; i++) {
        tlb[i].pagina = -1;
    }

    FILE *saida = fopen("correct.txt", "w");
    if (saida == NULL) {
        printf("Nao foi possivel criar correct.txt.\n");
        fclose(arquivo);
        return 1;
    }

    int faltas_pagina = 0;
    int acertos_tlb = 0;

    while (fgets(linha, sizeof linha, arquivo) != NULL) {
        numero_linha++;

        char *fim = linha;
        long endereco_lido = 0;
        int excedeu_limite = 0;

        while (*fim == ' ' || *fim == '\t') {
            fim++;
        }

        if (*fim == '+') {
            fim++;
        }

        char *inicio_numero = fim;

        while (*fim >= '0' && *fim <= '9') {
            int digito = *fim - '0';

            if (endereco_lido > (LONG_MAX - digito) / 10) {
                excedeu_limite = 1;
                break;
            }

            endereco_lido = endereco_lido * 10 + digito;
            fim++;
        }

        int encontrou_numero = fim != inicio_numero;
        int encontrou_quebra = 0;

        while (*fim == ' ' || *fim == '\t' || *fim == '\n' || *fim == '\r' || *fim == '\v' || *fim == '\f') {
            if (*fim == '\n') {
                encontrou_quebra = 1;
            }
            fim++;
        }

        int terminou_linha = (*fim == '\0');

        if ((encontrou_quebra == 0 && feof(arquivo) == 0) || encontrou_numero == 0 || terminou_linha == 0 || excedeu_limite == 1) {
            printf("Endereco invalido na linha %d.\n", numero_linha);
            fclose(saida);
            fclose(arquivo);
            return 1;
        }

        int endereco_logico = endereco_lido % TAM_ESPACO_VIRTUAL;
        int pagina = endereco_logico / TAM_PAGINA;
        int deslocamento = endereco_logico % TAM_PAGINA;

        int entrada_tlb = -1;
        int frame = -1;

        pthread_t threads[TOTAL_ENTRADAS_TLB];
        DadosBuscaTLB dados_busca[TOTAL_ENTRADAS_TLB];
        int threads_criadas = 0;
        int erro_threads = 0;

        for (int i = 0; i < TOTAL_ENTRADAS_TLB; i++) {
            dados_busca[i].pagina_procurada = pagina;
            dados_busca[i].pagina_da_entrada = tlb[i].pagina;

            if (pthread_create(&threads[i], NULL, buscar_entrada_tlb, &dados_busca[i]) == 0) {
                threads_criadas++;
            } else {
                erro_threads = 1;
                break;
            }
        }

        for (int i = 0; i < threads_criadas; i++) {
            if (pthread_join(threads[i], NULL) == 0) {
                if (dados_busca[i].encontrou == 1) {
                    entrada_tlb = i;
                    frame = tlb[i].frame;
                }
            } else {
                erro_threads = 1;
            }
        }

        if (erro_threads == 1) {
            printf("Erro ao executar a busca na TLB com threads.\n");
            fclose(saida);
            fclose(arquivo);
            return 1;
        }

        if (entrada_tlb == -1) {
            frame = tabela_paginas[pagina];
        } else {
            acertos_tlb++;
        }

        if (frame == -1) {
            faltas_pagina++;

            if (proximo_frame < TOTAL_FRAMES) {
                frame = proximo_frame;
                proximo_frame++;
            } else {
                if (usar_lru == 1) {
                    frame = 0;

                    for (int i = 1; i < TOTAL_FRAMES; i++) {
                        if (ultimo_acesso[i] < ultimo_acesso[frame]) {
                            frame = i;
                        }
                    }
                } else {
                    frame = proxima_vitima_fifo;
                    proxima_vitima_fifo = (proxima_vitima_fifo + 1) % TOTAL_FRAMES;
                }

                int pagina_removida = pagina_do_frame[frame];
                tabela_paginas[pagina_removida] = -1;
                for (int i = 0; i < TOTAL_ENTRADAS_TLB; i++) {
                    if (tlb[i].pagina == pagina_removida) {
                        tlb[i].pagina = -1;
                        break;
                    }
                }
            }

            int inicio_pagina = pagina * TAM_PAGINA;

            for (int i = 0; i < TAM_PAGINA; i++) {
                memoria[frame][i] = armazenamento[inicio_pagina + i];
            }

            tabela_paginas[pagina] = frame;
            pagina_do_frame[frame] = pagina;
        }

        ultimo_acesso[frame] = numero_linha;

        if (entrada_tlb == -1) {
            int destino = 0;

            for (int i = 0; i < TOTAL_ENTRADAS_TLB; i++) {
                if (tlb[i].pagina == -1) {
                    destino = i;
                    break;
                }

                if (tlb[i].ordem < tlb[destino].ordem) {
                    destino = i;
                }
            }

            tlb[destino].pagina = pagina;
            tlb[destino].frame = frame;
            tlb[destino].ordem = numero_linha;
            entrada_tlb = destino;
        }
        else if (usar_lru_tlb == 1) {
            tlb[entrada_tlb].ordem = numero_linha;
        }

        int endereco_fisico = frame * TAM_PAGINA + deslocamento;
        int valor = memoria[frame][deslocamento];
        if (valor >= 128) {
            valor -= 256;
        }

        if (fprintf(saida, "Virtual address: %ld TLB: %d Physical address: %d Value: %d\n",
                    endereco_lido, entrada_tlb, endereco_fisico, valor) < 0) {
            printf("Erro ao gravar correct.txt.\n");
            fclose(saida);
            fclose(arquivo);
            return 1;
        }
    }

    if (feof(arquivo) == 0) {
        printf("Erro ao ler o arquivo.\n");
        fclose(saida);
        fclose(arquivo);
        return 1;
    }

    double taxa_faltas = 0.0;
    double taxa_acertos = 0.0;

    if (numero_linha > 0) {
        taxa_faltas = 1.0 * faltas_pagina / numero_linha;
        taxa_acertos = 1.0 * acertos_tlb / numero_linha;
    }

    int resultado_resumo = fprintf(saida,
        "Number of Translated Addresses = %d\n"
        "Page Faults = %d\n"
        "Page Fault Rate = %.3f\n"
        "TLB Hits = %d\n"
        "TLB Hit Rate = %.3f\n",
        numero_linha, faltas_pagina, taxa_faltas, acertos_tlb, taxa_acertos);

    fclose(arquivo);
    int resultado_fechamento = fclose(saida);

    if (resultado_resumo < 0 || resultado_fechamento != 0) {
        printf("Erro ao gravar correct.txt.\n");
        return 1;
    }

    return 0;
}