# Simulador de Memória Virtual

Implementação em C para a disciplina de Infraestrutura de Software. O programa traduz endereços lógicos em endereços físicos e lê o valor do byte correspondente na memória simulada.

A memória física tem 128 frames de 256 bytes. A TLB tem 16 entradas, e a busca usa uma thread por entrada com `pthread`. O programa permite escolher FIFO ou LRU de forma independente para a substituição de páginas na RAM e de entradas na TLB.

## Compilação

No terminal do Linux ou do Ubuntu no WSL, execute na pasta do projeto:

```bash
make
```

O Makefile gera o executável `vm`.

## Execução

Coloque os arquivos `address.txt` e `BACKING_STORE.bin`, fornecidos pelo professor, na pasta do projeto. Execute:

```bash
./vm address.txt lru fifo
```

- `address.txt` contém um endereço lógico por linha.
- `lru` é o algoritmo de substituição de páginas na RAM.
- `fifo` é o algoritmo de substituição de entradas na TLB.

Os dois últimos argumentos aceitam `fifo` ou `lru`.

## Saída

O programa gera o arquivo `correct.txt` na pasta em que foi executado. Para cada endereço, ele registra o endereço lógico, o índice da entrada da TLB, o endereço físico e o valor do byte. No final, mostra o total de endereços traduzidos, as faltas de página, os acertos na TLB e suas respectivas taxas.

Com a entrada do professor, usando LRU na RAM e FIFO na TLB, o resultado foi de 1.000 endereços traduzidos, 539 faltas de página e 54 acertos na TLB.

## Limpeza

Para remover o executável gerado:

```bash
make clean
```