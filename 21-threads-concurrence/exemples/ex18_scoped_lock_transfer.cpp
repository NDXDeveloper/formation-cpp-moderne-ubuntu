/* ============================================================================
   Section 21.2.4 : std::scoped_lock - Transfert bancaire multi-mutex
   Description : Transferts croisés sans deadlock avec scoped_lock (C++17)
   Fichier source : 02.4-scoped-lock.md
   ============================================================================ */

#include <mutex>
#include <thread>
#include <vector>
#include <print>

struct Account {
    std::string name;
    double balance;
    std::mutex mtx;
};

void transfer(Account& from, Account& to, double amount) {
    // Un seul scoped_lock verrouille les deux comptes sans deadlock,
    // quel que soit l'ordre des arguments
    std::scoped_lock lock(from.mtx, to.mtx);

    if (from.balance >= amount) {
        from.balance -= amount;
        to.balance += amount;
        std::println("{} → {} : {:.2f}€", from.name, to.name, amount);
    }
}

int main() {
    Account alice{"Alice", 1000.0, {}};
    Account bob{"Bob", 1000.0, {}};
    Account charlie{"Charlie", 1000.0, {}};

    std::vector<std::thread> threads;

    // Transferts croisés simultanés — aucun deadlock possible
    for (int i = 0; i < 100; ++i) {
        threads.emplace_back(transfer, std::ref(alice), std::ref(bob), 10.0);
        threads.emplace_back(transfer, std::ref(bob), std::ref(alice), 10.0);
        threads.emplace_back(transfer, std::ref(bob), std::ref(charlie), 5.0);
        threads.emplace_back(transfer, std::ref(charlie), std::ref(alice), 5.0);
    }

    for (auto& t : threads) {
        t.join();
    }

    double total = alice.balance + bob.balance + charlie.balance;
    std::println("Soldes : Alice={:.2f}, Bob={:.2f}, Charlie={:.2f}",
                 alice.balance, bob.balance, charlie.balance);
    std::println("Total = {:.2f} (attendu : 3000.00)", total);
}
