/* ============================================================================
   Section 21.2.3 : std::unique_lock avec std::condition_variable
   Description : Producteur/consommateur avec condition_variable et unique_lock
   Fichier source : 02.3-unique-lock.md
   ============================================================================ */

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <print>

std::mutex mtx;
std::condition_variable cv;
std::queue<int> tasks;
bool finished = false;

void producer() {
    for (int i = 0; i < 10; ++i) {
        {
            std::lock_guard lock(mtx);  // lock_guard suffit ici
            tasks.push(i);
        }
        cv.notify_one();  // Réveiller un consommateur
    }

    {
        std::lock_guard lock(mtx);
        finished = true;
    }
    cv.notify_all();
}

void consumer() {
    while (true) {
        std::unique_lock lock(mtx);  // unique_lock obligatoire pour wait()

        cv.wait(lock, [] {
            return !tasks.empty() || finished;
        });

        if (tasks.empty() && finished) {
            break;  // Plus rien à traiter
        }

        int task = tasks.front();
        tasks.pop();
        lock.unlock();  // Libérer le mutex AVANT le traitement

        std::println("Traitement de la tâche {}", task);
    }
}

int main() {
    std::thread prod(producer);
    std::thread cons(consumer);

    prod.join();
    cons.join();

    std::println("Toutes les tâches traitées");
}
