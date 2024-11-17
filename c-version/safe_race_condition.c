#include <stdio.h>
#include <pthread.h>
#include <unistd.h> 
#include <stdlib.h> 
#include <time.h>   

int saldo = 1000; 
// O Mutex nao é associado a VAR ele em si é um controle de lock e unlock
// Quando eu executo pthread_mutex_lock, qualquer outra thread que tentar executar pthread_mutex_lock 
// Vai ser bloqueada até que a thread que executou pthread_mutex_lock execute pthread_mutex_unlock

pthread_mutex_t saldo_mutex; // Mutex para proteger o saldo

void creditar(int valor) {
    // Bloqueia o mutex
    pthread_mutex_lock(&saldo_mutex); 
    saldo += valor;
    printf("Creditado: %d | Saldo atual: %d\n", valor, saldo);
    // Libera o mutex
    pthread_mutex_unlock(&saldo_mutex);
}

void debitar(int valor) {
    // Bloqueia o mutex
    pthread_mutex_lock(&saldo_mutex); 
    if (saldo >= valor) {
        saldo -= valor;
        printf("Debitado: %d | Saldo atual: %d\n", valor, saldo);
    } else {
        printf("Saldo insuficiente para debitar: %d | Saldo atual: %d\n", valor, saldo);
    }
    // Libera o mutex
    pthread_mutex_unlock(&saldo_mutex); 
}

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
