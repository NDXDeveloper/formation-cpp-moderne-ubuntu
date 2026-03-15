/* ============================================================================
   Section 21.2.3 : std::unique_lock - Verrouillage simultané multi-mutex
   Description : Transfert bancaire avec defer_lock et std::lock()
   Fichier source : 02.3-unique-lock.md
   ============================================================================ */

#include <mutex>
#include <thread>
#include <print>
#include <string>

struct Account {
    std::string name;
    double balance;
    std::mutex mtx;
};

void transfer(Account& from, Account& to, double amount) {
    // 1. Créer les locks sans verrouiller
    std::unique_lock lock_from(from.mtx, std::defer_lock);
    std::unique_lock lock_to(to.mtx, std::defer_lock);

    // 2. Verrouiller les deux simultanément (évitement de deadlock)
    std::lock(lock_from, lock_to);

    // 3. Les deux mutex sont verrouillés — opération sûre
    if (from.balance >= amount) {
        from.balance -= amount;
        to.balance += amount;
        std::println("Transfert de {:.2f} : {} → {}", amount, from.name, to.name);
    }
}
// 4. Destruction : les deux mutex sont libérés automatiquement

int main() {
    Account alice{"Alice", 1000.0, {}};
    Account bob{"Bob", 500.0, {}};

    std::println("Avant : Alice={:.2f}, Bob={:.2f}", alice.balance, bob.balance);

    std::thread t1([&] { transfer(alice, bob, 200.0); });
    std::thread t2([&] { transfer(bob, alice, 50.0); });

    t1.join();
    t2.join();

    std::println("Après : Alice={:.2f}, Bob={:.2f}", alice.balance, bob.balance);
}
