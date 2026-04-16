# Estruturas de Arquivos em C

Nesse trabalho eu implementei algumas operações com arquivos binários em C, focando em busca, uso de índice e ordenação.

## Estrutura dos dados

O arquivo utiliza uma estrutura fixa de 300 bytes para cada registro:

```c
typedef struct {
    char logradouro[72];
    char bairro[72];
    char cidade[72];
    char uf[72];
    char sigla[2];
    char cep[8];
    char lixo[2];
} Endereco;
```

Isso permite acessar diretamente qualquer registro no arquivo usando a posição.

---

O trabalho foi dividido em três partes:

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

## Conclusão

Com esse trabalho foi possível entender melhor como funciona a manipulação de arquivos binários, principalmente em relação a busca eficiente, uso de índice e ordenação de grandes volumes de dados.
