#include <limits.h>
#include <stdio.h>

#define TAM_PAGINA 256
#define TAM_ESPACO_VIRTUAL (256L * TAM_PAGINA)

int main(int argc, char *argv[]){
    if (argc != 2) {
        printf("Uso nesta etapa: %s address.txt\n", argv[0]);
        return 1;
    }

    FILE *arquivo = fopen(argv[1], "r");
    if (arquivo == NULL) {
        printf("Nao foi possivel abrir o arquivo %s.\n", argv[1]);
        return 1;
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