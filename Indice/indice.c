#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// estrutura do arquivo original (cada registro tem 300 bytes)
typedef struct {
    char logradouro[72];
    char bairro[72];
    char cidade[72];
    char uf[72];
    char sigla[2];
    char cep[8];
    char lixo[2];
} Endereco;

// estrutura do índice (bem menor)
typedef struct {
    char cep[8];
    long posicao; // posição do registro no arquivo original
} Indice;

// função usada pelo qsort para ordenar pelo CEP
int compara(const void *a, const void *b) {
    return strncmp(((Indice *)a)->cep, ((Indice *)b)->cep, 8);
}

int main(int argc, char *argv[]) {
    FILE *f, *find;
    Endereco e;
    Indice idx;
    Indice *vet;
    long tam, n, i;
    long ini, fim, meio;
    int achou = 0;

    // precisa passar o CEP na execução
    if (argc != 2) {
        printf("Uso: %s [CEP]\n", argv[0]);
        return 1;
    }

    // abre o arquivo original
    f = fopen("cep.dat", "rb");
    if (f == NULL) {
        printf("Erro ao abrir cep.dat\n");
        return 1;
    }

    // descobre quantos registros existem
    fseek(f, 0, SEEK_END);
    tam = ftell(f);
    n = tam / sizeof(Endereco);
    rewind(f);

    // cria vetor de índice
    vet = (Indice *) malloc(n * sizeof(Indice));
    if (vet == NULL) {
        printf("Erro de memoria\n");
        fclose(f);
        return 1;
    }

    // preenche o índice lendo o arquivo original
    for (i = 0; i < n; i++) {
        fread(&e, sizeof(Endereco), 1, f);

        // copia só o CEP
        strncpy(vet[i].cep, e.cep, 8);

        // guarda onde esse registro está no arquivo
        vet[i].posicao = i;
    }

    // ordena o índice pelo CEP
    qsort(vet, n, sizeof(Indice), compara);

    // salva o índice em arquivo
    find = fopen("cep_indice.dat", "wb");
    if (find == NULL) {
        printf("Erro ao criar indice\n");
        free(vet);
        fclose(f);
        return 1;
    }

    fwrite(vet, sizeof(Indice), n, find);
    fclose(find);
    free(vet); // libera memória

    // abre o índice para fazer busca
    find = fopen("cep_indice.dat", "rb");
    if (find == NULL) {
        printf("Erro ao abrir indice\n");
        fclose(f);
        return 1;
    }

    ini = 0;
    fim = n - 1;

    // busca binária no índice
    while (ini <= fim) {
        meio = (ini + fim) / 2;

        // vai direto no registro do meio
        fseek(find, meio * sizeof(Indice), SEEK_SET);
        fread(&idx, sizeof(Indice), 1, find);

        // compara com o CEP digitado
        if (strncmp(argv[1], idx.cep, 8) == 0) {
            achou = 1;
            break;
        }
        else if (strncmp(argv[1], idx.cep, 8) < 0) {
            fim = meio - 1;
        }
        else {
            ini = meio + 1;
        }
    }

    if (achou) {
        // usa a posição do índice para ir no arquivo original
        fseek(f, idx.posicao * sizeof(Endereco), SEEK_SET);
        fread(&e, sizeof(Endereco), 1, f);

        // imprime os dados
        printf("CEP encontrado\n");
        printf("CEP: %.8s\n", e.cep);
        printf("Logradouro: %.72s\n", e.logradouro);
        printf("Bairro: %.72s\n", e.bairro);
        printf("Cidade: %.72s\n", e.cidade);
        printf("UF: %.72s\n", e.uf);
    } else {
        printf("CEP nao encontrado\n");
    }

    fclose(f);
    fclose(find);

    return 0;
}