# Estruturas de Arquivos em C

**Autores:** Matheus Jasbick Conceição Machado e Thiago Souza da Silva

Nesse trabalho foram implementadas operações sobre arquivos em C. A maior parte trabalha com um arquivo binário de registros de tamanho fixo (endereços de CEP), com foco em busca, uso de índice e ordenação. Há também uma parte separada que processa um arquivo CSV grande de dados de COVID-19 sem carregar tudo na memória.

## Estrutura dos dados (arquivo de CEP)

O arquivo de CEP usa uma estrutura fixa de 300 bytes para cada registro:

```c
typedef struct {
    char logradouro[72];
    char bairro[72];
    char cidade[72];
    char estado[72];   /* nome do estado, ex.: "ACRE" */
    char uf[2];        /* sigla de 2 letras, ex.: "AC" */
    char cep[8];
    char lixo[2];
} Endereco;
```

Isso permite acessar diretamente qualquer registro no arquivo usando a posição (posição em bytes = RRN × 300).

---

O trabalho foi dividido em cinco partes:

## 1. Busca binária

Foi feita uma busca binária diretamente no arquivo `cep_ordenado.dat`.

Como o arquivo já está ordenado pelo CEP, é possível acessar posições específicas com `fseek` e ir reduzindo o espaço de busca pela metade até encontrar o valor desejado.

---

## 2. Índice

Também foi criado um arquivo de índice contendo:
- CEP
- posição do registro no arquivo original

A ideia aqui é não precisar percorrer o arquivo inteiro.

O índice é ordenado e a busca binária é feita nele.  
Quando o CEP é encontrado, a posição armazenada é usada para acessar diretamente o registro no arquivo original.

---

## 3. Ordenação externa

A ordenação foi feita usando o conceito de ordenação externa.

O arquivo é dividido em blocos menores, cada bloco é carregado na memória e ordenado. Depois disso, os blocos são intercalados até formar um único arquivo final ordenado.

O resultado final é o arquivo `cep_ordenado_final.dat`.

---

## 4. Processamento de CSV — COVID-19

Essa parte processa o arquivo `owid-covid-data.csv` (Our World in Data) e soma, por país da América do Sul, o total de casos e de mortes.

O arquivo é grande (~84 MB), então ele não é carregado inteiro na memória. A leitura é feita em blocos de 8 KB, e cada bloco é passado para um parser de CSV escrito como máquina de estados (*table-driven*), que monta uma linha por vez e chama uma função de *callback* a cada registro completo. Assim o uso de memória fica constante, independente do tamanho do arquivo.

Para cada linha, é feito um filtro pela coluna `continent == "South America"` e os valores são acumulados por país nesta struct:

```c
typedef struct {
    char location[64];
    double total_cases;
    double total_deaths;
} CountryData;
```

As colunas `total_cases` e `total_deaths` são acumuladas (o valor de cada dia já inclui os dias anteriores), então o total de um país é o **último valor informado** — não a soma das linhas nem o maior valor já visto. Usar o maior valor daria erro quando a série é corrigida para baixo: o Chile, por exemplo, chega a um pico de 64.497 mortes que depois é corrigido para 61.508. Pegando o último valor por data, a soma dos países bate exatamente com o agregado `South America` da própria base: **68.630.314 casos** e **1.352.954 mortes**.

---

## 5. Junção (join) com índice em árvore B

Essa parte calcula a **interseção** de duas amostras do arquivo de CEP usando um índice em árvore B gravado em disco. O processo tem três passos, encadeados pelo `main.c`:

1. **Amostragem** — de `cep.dat` são geradas duas amostras, cada uma com exatamente 80% dos registros, escolhidos ao acaso e gravados em ordem embaralhada. Isso é feito com um embaralhamento de Fisher-Yates sobre um vetor de índices dos registros: depois de embaralhar, os primeiros 80% dos índices formam um subconjunto aleatório, já em ordem aleatória. As duas amostras (`sample1.dat` e `sample2.dat`) vêm de dois embaralhamentos independentes, então se sobrepõem só em parte (cerca de 64% dos registros caem nas duas), que é o que dá sentido ao passo do join.
2. **Índice** — para cada registro de `sample1.dat`, o CEP (8 bytes na posição 290 do registro) é inserido numa árvore B gravada em `index.dat`, junto com a posição do registro no arquivo.
3. **Join** — para cada registro de `sample2.dat`, o CEP é buscado no índice; os que aparecem nas duas amostras são gravados em `result.dat`. É a interseção, com uma busca na árvore por registro.

A árvore B vive inteira em um único arquivo: o byte 0 guarda um cabeçalho que aponta para a página raiz, e cada página é um bloco de tamanho fixo cujo deslocamento em bytes serve como "ponteiro". Cada elemento guarda a chave (o CEP) e a posição do registro no arquivo de dados. A inserção divide (*split*) uma página cheia e promove a chave do meio para o pai, aumentando a altura da árvore em um nível quando a própria raiz precisa dividir.

Diferente da Parte 2 (índice ordenado + busca binária), aqui o índice é uma árvore B: as duas resolvem o mesmo problema — achar um CEP sem varrer o arquivo inteiro — por caminhos diferentes.

---

## Conclusão

Com esse trabalho foi possível entender melhor como funciona a manipulação de arquivos em C: no caso dos arquivos binários de registro fixo, a busca eficiente, o uso de índice (tanto por busca binária quanto por árvore B) e a ordenação de grandes volumes de dados, além da junção de dois arquivos por interseção usando o índice; e, no caso do CSV, o processamento de um arquivo grande em fluxo (*streaming*), com uso de memória constante independentemente do tamanho da entrada.
