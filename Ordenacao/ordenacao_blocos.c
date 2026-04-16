#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// estrutura do registro (300 bytes)
typedef struct {
    char logradouro[72];
    char bairro[72];
    char cidade[72];
    char uf[72];
    char sigla[2];
    char cep[8];
    char lixo[2];
} Endereco;

// compara dois registros pelo CEP (usado no qsort)
int compara(const void *a, const void *b) {
    return strncmp(((Endereco *)a)->cep, ((Endereco *)b)->cep, 8);
}

// intercala dois arquivos ordenados em um terceiro
void intercala_arquivos(const char *a_nome, const char *b_nome, const char *saida_nome) {
    FILE *a, *b, *saida;
    Endereco ea, eb;

    int leu_a, leu_b;

    a = fopen(a_nome, "rb");
    b = fopen(b_nome, "rb");
    saida = fopen(saida_nome, "wb");

    if (a == NULL || b == NULL || saida == NULL) {
        printf("Erro ao abrir arquivos\n");
        return;
    }

    // lê o primeiro registro de cada arquivo
    leu_a = fread(&ea, sizeof(Endereco), 1, a);
    leu_b = fread(&eb, sizeof(Endereco), 1, b);

    // enquanto os dois ainda têm dados
    while (leu_a == 1 && leu_b == 1) {

        // pega o menor CEP e escreve
        if (strncmp(ea.cep, eb.cep, 8) <= 0) {
            fwrite(&ea, sizeof(Endereco), 1, saida);
            leu_a = fread(&ea, sizeof(Endereco), 1, a);
        } else {
            fwrite(&eb, sizeof(Endereco), 1, saida);
            leu_b = fread(&eb, sizeof(Endereco), 1, b);
        }
    }

    // copia o restante do arquivo A (se sobrar)
    while (leu_a == 1) {
        fwrite(&ea, sizeof(Endereco), 1, saida);
        leu_a = fread(&ea, sizeof(Endereco), 1, a);
    }

    // copia o restante do arquivo B (se sobrar)
    while (leu_b == 1) {
        fwrite(&eb, sizeof(Endereco), 1, saida);
        leu_b = fread(&eb, sizeof(Endereco), 1, b);
    }

    fclose(a);
    fclose(b);
    fclose(saida);
}

// função principal de ordenação externa
void ordenacao_externa(const char *arquivo, int k) {
    FILE *f, *fb;
    Endereco *buffer;

    long total, por_bloco, qtd;
    int i;

    char nomes[100][50];

    // abre o arquivo original
    f = fopen(arquivo, "rb");
    if (f == NULL) {
        printf("Erro ao abrir arquivo\n");
        return;
    }

    // descobre quantos registros existem
    fseek(f, 0, SEEK_END);
    total = ftell(f) / sizeof(Endereco);
    rewind(f);

    // define tamanho de cada bloco
    por_bloco = total / k;

    printf("Criando blocos...\n");

    // ===== ETAPA 1: dividir e ordenar blocos =====
    for (i = 0; i < k; i++) {

        // último bloco pega o resto
        if (i == k - 1)
            qtd = total - (i * por_bloco);
        else
            qtd = por_bloco;

        buffer = malloc(qtd * sizeof(Endereco));
        if (buffer == NULL) {
            printf("Erro de memoria\n");
            fclose(f);
            return;
        }

        // lê bloco inteiro pra memória
        fread(buffer, sizeof(Endereco), qtd, f);

        // ordena o bloco
        qsort(buffer, qtd, sizeof(Endereco), compara);

        // salva bloco em arquivo
        sprintf(nomes[i], "bloco_%d.dat", i);
        fb = fopen(nomes[i], "wb");

        fwrite(buffer, sizeof(Endereco), qtd, fb);

        fclose(fb);
        free(buffer);
    }

    fclose(f);

    printf("Intercalando blocos...\n");

    // ===== ETAPA 2: intercalação =====
    int ativos = k;
    int rodada = 0;

    while (ativos > 1) {
        int novos = 0;

        for (i = 0; i < ativos; i += 2) {

            // se tem par, intercala
            if (i + 1 < ativos) {
                char nome_saida[50];

                sprintf(nome_saida, "temp_%d_%d.dat", rodada, novos);

                intercala_arquivos(nomes[i], nomes[i + 1], nome_saida);

                // remove arquivos antigos
                remove(nomes[i]);
                remove(nomes[i + 1]);

                strcpy(nomes[novos], nome_saida);
                novos++;
            } else {
                // se sobrar um, passa direto
                strcpy(nomes[novos], nomes[i]);
                novos++;
            }
        }

        ativos = novos;
        rodada++;

        printf("Rodada %d feita, restam %d arquivos\n", rodada, ativos);
    }

    // renomeia resultado final
    rename(nomes[0], "cep_ordenado_final.dat");

    printf("Arquivo final criado: cep_ordenado_final.dat\n");
}

int main() {
    ordenacao_externa("cep.dat", 8); // 8 blocos (potência de 2)
    return 0;
}