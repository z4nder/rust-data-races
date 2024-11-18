## POC Rust Data Races: Uma Comparação com C  
**Threads Safety: Data Races de C ao Rust**  

## Índice  
- [1. Introdução](#1-introducao)  
- [2. Threads e Concorrência](#2-threads-e-concorrencia)  
- [3. Implementação em C](#3-implementacao-em-c)  
  - [3.1. Código sem Proteção Contra Race Conditions](#3-1-codigo-sem-protecao)  
  - [3.2. Corrigindo com Mutex](#3-2-corrigindo-com-mutex)  
- [4. Implementação em Rust](#4-implementacao-em-rust)  
  - [4.1. Problema com Race Conditions](#4-1-problema-rust)  
  - [4.2. Resolução com Mutex e Arc](#4-2-resolucao-com-mutex)  
  - [4.3. Mutex vs. RwLock](#4-3-mutex-vs-rwlock)  
- [5. Conclusão](#5-conclusao)  
- [6. Referências](#6-referencias)  

---

### 1. Introdução <a name="1-introducao"></a>  
Em computação, **threads** são usadas para dividir tarefas de software em subtarefas que podem ser executadas concorrentemente. Ao usar **threads**, ganhamos tempo de processamento e aproveitamos melhor os recursos da máquina, mas essa concorrência traz desafios, como **race conditions** que podem gerar inconsistências graves nos dados.

Nesta **POC** (Proof of Concept), exploraremos como a linguagem **Rust** trata as **race conditions**, comparando-a com **C**, uma linguagem amplamente usada, mas com menos garantias de segurança para concorrência.

---

### 2. Threads e Concorrência <a name="2-threads-e-concorrencia"></a>  
**Threads** são unidades de execução que permitem processar tarefas simultaneamente. Podemos pensar em threads como fluxos independentes de execução dentro de um programa, ilustrados na imagem abaixo:  

<h1 align="left">
  <img src="https://dev-to-uploads.s3.amazonaws.com/uploads/articles/2uoyr3ps4icgof9t8xc0.png" width="600px" />
</h1>
Embora as threads tragam vantagens de desempenho, elas introduzem riscos, especialmente ao acessar recursos compartilhados.

---

### 3. Implementação em C <a name="3-implementacao-em-c"></a>  
Vamos criar um sistema simples em **C**:  
1. Um saldo inicial de 1000.  
2. Um conjunto de transações que podem ser créditos ou débitos.  
3. Processamento paralelo dessas transações usando threads.

#### 3.1. Código sem Proteção Contra Race Conditions <a name="3-1-codigo-sem-protecao"></a>  
```c
int saldo = 1000; 

void creditar(int valor) {
    int tmp_saldo = saldo;

    sleep(1); // Delay simulado
    
    saldo += tmp_saldo + valor;
}

void debitar(int valor) {
    int temp = saldo;

    sleep(1); // Delay simulado

    if (temp >= valor) {
        saldo = temp - valor;
    }
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

    for (int i = 0; i < num_transactions; i++) {
        pthread_create(&threads[i], NULL, processar_transacao, &transactions[i]); // Cria uma thread para cada transação
    }

    for (int i = 0; i < num_transactions; i++) {
        pthread_join(threads[i], NULL); // Aguarda todas as threads terminarem
    }

    printf("Saldo final da conta: %d\n", saldo);
    return 0;
}
```
Ao optarmos por um ambiente com **processamento multithreading** pode acontecer o que chamamos de **race conditions**, no momento em que 2 threads acessam e modificam um mesmo valor temos uma condição de corrida. Esse problema acontece pois não é garantido uma sincronização do valor acessado em cada thread devido à concorrência entre as chamadas. 

Ao executar várias vezes esse código, o saldo final varia, pois threads acessam e alteram `saldo` simultaneamente.
<h1 align="left">
  <img src="https://dev-to-uploads.s3.amazonaws.com/uploads/articles/ts89527lobltsjkcv3cf.png" width="600px" />
</h1>

---

#### 3.2. Corrigindo com Mutex <a name="3-2-corrigindo-com-mutex"></a>  
```c
int saldo = 1000; 
pthread_mutex_t saldo_mutex; // Mutex para proteger o saldo

void creditar(int valor) { 
    pthread_mutex_lock(&saldo_mutex); // Bloqueia o mutex
    int tmp_saldo = saldo;

    sleep(1); // Delay simulado

    saldo = tmp_saldo + valor;

    pthread_mutex_unlock(&saldo_mutex); // Libera o mutex
}

void debitar(int valor) {
    pthread_mutex_lock(&saldo_mutex); // Bloqueia o mutex
    int tmp_saldo = saldo;

    sleep(1); // Delay simulado

    if (tmp_saldo >= valor) {
        saldo = tmp_saldo - valor;
    }

    pthread_mutex_unlock(&saldo_mutex);  // Libera o mutex
}
```
Mutex é um primitivo de sincronização que garante que apenas um thread tenha acesso a um recurso compartilhado por vez. O acrônimo **mutex** vem do termo em inglês _mutual exclusion_, que significa "exclusão mútua". 

Quando uma thread adquire um **mutex**, qualquer outra thread que tente adquirir o mesmo **mutex** é suspenso até que a primeira thread libere o **mutex**. Isso evita que dois ou mais processos(threads) tenham acesso simultâneo ao recurso compartilhado. 

<h1 align="left">
  <img src="https://dev-to-uploads.s3.amazonaws.com/uploads/articles/fzmz96x1bzhu3id7l7ft.png" width="600px" />
</h1>

### 4. Implementação em Rust <a name="4-implementacao-em-rust"></a>  
```
Rust’s rich type system and ownership model guarantee memory-safety and thread-safety — enabling you to eliminate many classes of bugs at compile-time.
```
Pensar em Rust como uma linguagem ausente de **data race** não é produtivo, mas podemos entender como as **structs** e seu compilador contribuem trazendo recursos ótimos para segurança de memória e thread. 

Rust trata **race conditions** com garantias em tempo de compilação, utilizando recursos como **ownership**, **borrowing** e estruturas seguras para concorrência:  
- **Arc**: Compartilhamento seguro de dados imutáveis.  
- **Mutex** e **RwLock**: Controle de acesso para dados mutáveis.

#### 4.1. Problema com Race Conditions <a name="4-1-problema-rust"></a>  
Sem o uso das structs Arc e Mutex
```rust
fn main() {
    let mut saldo = 1000; // saldo mutável, mas sem proteção

    let handle1 = thread::spawn(move || {
        saldo += 100;  // erro: `saldo` é movido para esta thread sem proteção
    });

    let handle2 = thread::spawn(move || {
        saldo -= 50;  // erro: `saldo` é movido para esta thread sem proteção
    });

    handle1.join().unwrap();
    handle2.join().unwrap();
}
```
Rust não permite o acesso direto a um dado **mutável** (saldo) a partir de várias **threads** sem proteção.
O compilador vai gerar um erro porque saldo está sendo movido para várias threads (_handle1_ e _handle2_) sem um mecanismo seguro.
Mensagem de erro que será exibida é:
```shell
error[E0382]: use of moved value: `saldo`
```

#### 4.2. Resolução com Mutex e Arc <a name="4-2-resolucao-com-mutex"></a>  
Usando `Mutex` e `Arc` conseguimos compilar e executar nosso código, com os problemas de **race condition** tratados.
```rust
struct ContaBancaria {
    saldo: i32,
}

impl ContaBancaria {
    fn creditar(&mut self, valor: i32) {
        let tmp_saldo = self.saldo;
        thread::sleep(time::Duration::from_secs(1));
        self.saldo = tmp_saldo + valor;
    }

    fn debitar(&mut self, valor: i32) {
        let tmp_saldo = self.saldo;

        thread::sleep(time::Duration::from_secs(1));

        if tmp_saldo >= valor {
            self.saldo = tmp_saldo - valor;
        }
    }

    fn consultar_saldo(&self) -> i32 {
        self.saldo
    }
}

fn main() {
    let conta = Arc::new(Mutex::new(ContaBancaria { saldo: 1000 }));  // Cria a conta com Arc

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

    
    for handle in handles {
        handle.join().unwrap(); // Espera todas as threads terminarem
    }

    let saldo_final = conta.lock().unwrap().consultar_saldo();
    println!("Saldo final da conta: {}", saldo_final);
}
```

#### 4.3. Mutex vs. RwLock <a name="4-3-mutex-vs-rwlock"></a>  
Mutex e RwLock são usados para tratar **race conditions**, cada um com vantagens específicas:

Mutex: Garante acesso exclusivo de um recurso para uma thread, bloqueando o acesso das outras até seja liberado. É simples e eficaz, mas mesmo leituras bloqueiam o recurso, tornando-o **menos eficiente em cenários com muitas leituras**.

RwLock: Permite múltiplas leituras simultâneas com .read() e restringe a escrita exclusiva com .write(). É **Ideal para cenários com predominância de leituras**, pois melhora o desempenho ao permitir paralelismo nas operações de leitura.

---

### 5. Conclusão <a name="5-conclusao"></a>  
A comparação entre C e Rust destaca abordagens diferentes para resolver **race conditions**. Enquanto C exige atenção para evitar erros de condições de corrida, Rust reduz esses riscos em tempo de compilação, por meio de ferramentas como Mutex, RwLock e Arc além do modelo de ownership. Isso não apenas torna o código mais seguro, mas também **reduz a carga mental do programador** evitando bugs silenciosos.

Em resumo, Rust se posiciona como uma excelente escolha para o desenvolvimento de sistemas **concorrentes**, oferecendo segurança e confiabilidade.

---

### 6. Referências <a name="6-referencias"></a>  
- https://en.wikipedia.org/wiki/Race_condition
- https://blog.bughunt.com.br/o-que-sao-vulnerabilidades-race-condition/
- https://medium.com/cwi-software/spring-boot-race-condition-e-ambiente-multi-thread-263b21e0042e
- https://learn.microsoft.com/en-us/troubleshoot/developer/visualstudio/visual-basic/language-compilers/race-conditions-deadlocks
- https://www.reddit.com/r/rust/comments/18faxjg/understanding_threadsafety_vs_race_conditions/?rdt=52263
- https://doc.rust-lang.org/nomicon/races.html
- https://news.ycombinator.com/item?id=23599598