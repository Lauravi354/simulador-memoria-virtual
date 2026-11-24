#include <stdio.h>

#define TAM_PAGINA 256

int main(void){
    
int endereco_logico = 1802;

int pagina = endereco_logico / TAM_PAGINA;
int deslocamento = endereco_logico % TAM_PAGINA;

printf("Pagina: %d\n", pagina);
printf("Deslocamento: %d\n", deslocamento);
}