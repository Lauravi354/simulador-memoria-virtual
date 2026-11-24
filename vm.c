#include <limits.h>
#include <stdio.h>
#include <string.h>

#define TAM_PAGINA 256
#define TAM_ESPACO_VIRTUAL (256L * TAM_PAGINA)
#define TOTAL_PAGINAS 256
#define TOTAL_FRAMES 128

int main(int argc, char *argv[]){
    if (argc != 3) {
        printf("Uso nesta etapa: %s address.txt fifo|lru\n", argv[0]);
        return 1;
    }

    int usar_fifo = strcmp(argv[2], "fifo") == 0;
    int usar_lru = strcmp(argv[2], "lru") == 0;

    if (usar_fifo == 0 && usar_lru == 0) {
        printf("Algoritmo invalido. Use fifo ou lru.\n");
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

    if (fread(armazenamento, 1, sizeof armazenamento, disco) < sizeof armazenamento) {
        printf("Nao foi possivel ler todo o BACKING_STORE.bin.\n");
        fclose(disco);
        fclose(arquivo);
        return 1;
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
            fclose(arquivo);
            return 1;
        }

        int endereco_logico = endereco_lido % TAM_ESPACO_VIRTUAL;
        int pagina = endereco_logico / TAM_PAGINA;
        int deslocamento = endereco_logico % TAM_PAGINA;

        int frame = tabela_paginas[pagina];

        if (frame == -1) {
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
            }

            int inicio_pagina = pagina * TAM_PAGINA;

            for (int i = 0; i < TAM_PAGINA; i++) {
                memoria[frame][i] = armazenamento[inicio_pagina + i];
            }

            tabela_paginas[pagina] = frame;
            pagina_do_frame[frame] = pagina;
        }

        ultimo_acesso[frame] = numero_linha;

        int endereco_fisico = frame * TAM_PAGINA + deslocamento;
        int valor = memoria[frame][deslocamento];

        printf("%ld -> pagina %d, frame %d, endereco fisico %d, valor %d\n", endereco_lido, pagina, frame, endereco_fisico, valor);

        printf("%ld -> pagina %d, deslocamento %d\n", endereco_lido, pagina, deslocamento);
    }

    if (feof(arquivo) == 0) {
        printf("Erro ao ler o arquivo.\n");
        fclose(arquivo);
        return 1;
    }

    fclose(arquivo);
    return 0;
}