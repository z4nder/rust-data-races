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
