#include <stdio.h>
#include <pthread.h>
#include <unistd.h> 
#include <stdlib.h> 
#include <time.h>   

int saldo = 1000; 

void creditar(int valor) {
    int tmp_saldo = saldo;

    // Delay aleatório entre 1 e 10
    int random_sleep_time = rand() % 10;
    sleep(random_sleep_time);
    
    saldo = tmp_saldo + valor;
    printf("Creditado: %d | Saldo atual: %d\n", valor, saldo);
}

void debitar(int valor) {
    int temp = saldo;

    // Delay aleatório entre 1 e 10
    int random_sleep_time = rand() % 10;
    sleep(random_sleep_time);

    if (temp >= valor) {
        saldo = temp - valor;
        printf("Debitado: %d | Saldo atual: %d\n", valor, saldo);
    } else {
        printf("Saldo insuficiente para debitar: %d | Saldo atual: %d\n", valor, saldo);
    }
}

// Função que cada thread executará
void* processar_transacao(void* arg) {
    int valor = *(int*)arg;

    if (valor > 0) {
        creditar(valor);
    } else {
        debitar(abs(valor));
    }

    return NULL;
}

int main() {
    int transactions[] = {100, -50, 200, -150, 300, -200, 150, -100, 50, -50};
    int num_transactions = sizeof(transactions) / sizeof(transactions[0]);

    pthread_t threads[num_transactions];

    // Cria uma thread para cada transação
    for (int i = 0; i < num_transactions; i++) {
        pthread_create(&threads[i], NULL, processar_transacao, &transactions[i]);
    }

    // Aguarda todas as threads terminarem
    for (int i = 0; i < num_transactions; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Saldo final da conta: %d\n", saldo);
    return 0;
}
