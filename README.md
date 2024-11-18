# POC Rust Data Races: Uma Comparação com C  
**Threads Safety: Data Races de C ao Rust**  

## Introdução  
Em computação, **threads** são usadas para dividir tarefas de software em subtarefas que podem ser executadas concorrentemente. Ao usar **threads** ganhamos tempo de processamento e aproveitamos melhor os recursos da máquina, mas essa concorrência traz desafios como **race conditions**, que podem gerar inconsistências graves nos dados.

Nesta POC (Proof of Concept), exploraremos como a linguagem **Rust** trata as **race conditions**, comparando com **C**, uma linguagem amplamente usada, mas com menos garantias de segurança para concorrência.

---

## Threads e Concorrência  
**Threads** são unidades de execução que permitem processar tarefas simultaneamente. Podemos pensar em threads como fluxos independentes de execução dentro de um programa, ilustrados na imagem abaixo:  

<h1 align="left">
  <img src="https://dev-to-uploads.s3.amazonaws.com/uploads/articles/2uoyr3ps4icgof9t8xc0.png" width="600px" />
</h1>
Embora threads tragam vantagens de desempenho, elas introduzem riscos, especialmente ao acessar recursos compartilhados.

## Implementação em C  
Vamos criar um sistema simples em **C**:  
1. Um saldo inicial de 1000.  
2. Um conjunto de transações que podem ser créditos ou débitos.  
3. Processamento paralelo dessas transações usando threads.

#### Código sem Proteção Contra Race Conditions  
```c
int saldo = 1000;

int main() {
    int transactions[] = {100, -50, 200, -150, 300, -200, 150, -100, 50, -50};
    int num_transactions = sizeof(transactions) / sizeof(transactions[0]);

    pthread_t threads[num_transactions];

    for (int i = 0; i < num_transactions; i++) {
        pthread_create(&threads[i], NULL, processar_transacao, &transactions[i]);
    }

    for (int i = 0; i < num_transactions; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Saldo final: %d\n", saldo);
    return 0;
}
```

### Problema(race condition):
Ao optarmos por um ambiente com **processamento multi threads** pode acontecer o que chamamos de **race conditions**, no momento em que 2 threads acessam e modificam um mesmo valor temos uma condição de corrida, esse problema acontece, pois não é garantido um sincronismo do valor acessado e modificado em cada thread devido à concorrência entre as chamada. 

Ao executar várias vezes esse código, o saldo final varia, pois threads acessam e alteram `saldo` simultaneamente.
<h1 align="left">
  <img src="./assets/rc.png" width="600px" />
</h1>

### Corrigindo com Mutex  
Mutex é um primitivo de sincronização que garante que apenas um thread tenha acesso a um recurso compartilhado por vez. O acrônimo **mutex** vem do termo em inglês mutual exclusion, que significa "exclusão mútua". 

Quando um thread adquire um **mutex**, qualquer outro thread que tente adquirir o mesmo **mutex** é suspenso até que o primeiro thread libere o **mutex**. Isso evita que dois ou mais processos, ou threads tenham acesso simultâneo ao recurso compartilhado, o que é conhecido como seção crítica. 
 
O mutex é uma técnica de programação concorrente que garante a integridade dos dados compartilhados e evita raças de dados. 

<h1 align="left">
  <img src="./assets/mutex.png" width="600px" />
</h1>

```c
pthread_mutex_t saldo_mutex; // Mutex para proteger o saldo

void creditar(int valor) {
    // Bloqueia o mutex
    pthread_mutex_lock(&saldo_mutex); 
    saldo += valor;
    printf("Creditado: %d | Saldo atual: %d\n", valor, saldo);
    // Libera o mutex
    pthread_mutex_unlock(&saldo_mutex);
}
```

---

## Implementação em Rust  
```
Rust’s rich type system and ownership model guarantee memory-safety and thread-safety — enabling you to eliminate many classes of bugs at compile-time. (Texto altamente controverso)
```
Pensar em Rust como uma linguagem ausente de data race não é produtivo, mas podemos entender como os tipos lineares  e seu compilador contribuem trazendo recursos ótimos para segurança de memória e thread. 

Rust resolve **race conditions** com garantias em tempo de compilação, utilizando recursos como **ownership**, **borrowing**, e estruturas seguras de concorrência:  
- **Arc**: Compartilhamento seguro de dados imutáveis.  
- **Mutex** e **RwLock**: Controle de acesso para dados mutáveis.

### Problema(race condition):
Sem o uso das structs Arc e Mutex
```rust
fn main() {
    // saldo mutável, mas sem proteção
    let mut saldo = 1000; 

    // erro: `saldo` é movido para esta thread sem proteção
    let handle1 = thread::spawn(move || {
        saldo += 100; 
    });

    // erro: `saldo` é movido para esta thread sem proteção
    let handle2 = thread::spawn(move || {
        saldo -= 50; 
    });

    handle1.join().unwrap();
    handle2.join().unwrap();

    println!("Saldo final: {}", saldo);
}
```
Rust não permite o acesso direto a um dado mutável(saldo) a partir de várias threads sem proteção.
O compilador vai gerar um erro porque saldo está sendo movido para várias threads (handle1 e handle2) sem um mecanismo seguro.
Mensagem de erro que você pode ver:
```shell
plaintext
Copy code
error[E0382]: use of moved value: `saldo`
```

### Resolução com `Mutex` e `Arc` 
Usando `Mutex` e `Arc` conseguimos compilar e executar nosso código, podemos ver que os problemas de **race condition** foram tratados.
```rust
use rand::Rng;
use std::sync::{Arc, Mutex};
use std::{thread, time};

struct ContaBancaria {
    saldo: i32,
}

impl ContaBancaria {
    fn creditar(&mut self, valor: i32) {
        self.saldo += valor;
        println!("Creditado: {} | Saldo atual: {}", valor, self.saldo);
    }

    fn debitar(&mut self, valor: i32) {
        if self.saldo >= valor {
            self.saldo -= valor;
            println!("Debitado: {} | Saldo atual: {}", valor, self.saldo);
        } else {
            println!(
                "Saldo insuficiente para debitar: {} | Saldo atual: {}",
                valor, self.saldo
            );
        }
    }

    fn consultar_saldo(&self) -> i32 {
        self.saldo
    }
}

fn main() {
    rust_multi_threads_transaction();
}

fn rust_multi_threads_transaction() {
    // Cria a conta com Arc
    let conta = Arc::new(Mutex::new(ContaBancaria { saldo: 1000 }));

    let mut handles = vec![];
    let transactions = [100, -50, 200, -150, 300, -200, 150, -100, 50, -50];

    for transaction in transactions {
        let conta = Arc::clone(&conta);

        let handle = thread::spawn(move || {
            let random_sleep_time = rand::thread_rng().gen_range(0..10);
            thread::sleep(time::Duration::from_secs(random_sleep_time));

            if transaction > 0 {
                conta.lock().unwrap().creditar(transaction);
            } else {
                conta.lock().unwrap().debitar(transaction.abs());
            }
        });

        handles.push(handle);
    }

    // Espera todas as threads terminarem
    for handle in handles {
        handle.join().unwrap();
    }

    let saldo_final = conta.lock().unwrap().consultar_saldo();
    println!("Saldo final da conta: {}", saldo_final);
}
```

### Rust Mutex x RwLock
Em Rust, tanto **Mutex** quanto **RwLock** são usados para tratar race conditions, cada um com vantagens específicas.

#### Mutex (Mutual Exclusion)
O Mutex garante que apenas uma thread tenha acesso exclusivo a um recurso compartilhado, bloqueando outras threads até que o acesso seja liberado.

Como funciona:

Bloqueio exclusivo: Ao chamar .lock(), a thread bloqueia o Mutex e obtém acesso exclusivo ao recurso.
Liberação automática: O bloqueio é liberado automaticamente quando sai do escopo.
Aguarda pelo acesso: Threads que tentarem acessar o recurso bloqueado esperam até que o Mutex seja liberado.
Ponto negativo: Mesmo operações de leitura bloqueiam o acesso, o que pode ser ineficiente em cenários com muitas leituras e poucas escritas.

#### RwLock (Read-Write Lock)
O RwLock permite que múltiplas threads leiam o recurso simultaneamente, mas restringe o acesso exclusivo para escrita.

Como funciona:

Leitura com .read(): Várias threads podem ler o recurso ao mesmo tempo, desde que não haja uma escrita em andamento.
Escrita com .write(): Apenas uma thread pode escrever, bloqueando todas as leituras e escritas enquanto estiver acessando o recurso.
Vantagem: É ideal para cenários onde há muitas leituras e poucas escritas, maximizando o desempenho ao permitir paralelismo nas leituras.

### Conclusão  
A comparação evidencia como Rust é projetado para evitar **race conditions**, enquanto **C** exige maior cuidado do desenvolvedor. Apesar de maior complexidade inicial, Rust entrega segurança e previsibilidade, características essenciais em sistemas concorrentes modernos.


### Refs
- https://en.wikipedia.org/wiki/Race_condition
- https://blog.bughunt.com.br/o-que-sao-vulnerabilidades-race-condition/
- https://medium.com/cwi-software/spring-boot-race-condition-e-ambiente-multi-thread-263b21e0042e
- https://learn.microsoft.com/en-us/troubleshoot/developer/visualstudio/visual-basic/language-compilers/race-conditions-deadlocks
- https://www.reddit.com/r/rust/comments/18faxjg/understanding_threadsafety_vs_race_conditions/?rdt=52263
- https://doc.rust-lang.org/nomicon/races.html
- https://news.ycombinator.com/item?id=23599598