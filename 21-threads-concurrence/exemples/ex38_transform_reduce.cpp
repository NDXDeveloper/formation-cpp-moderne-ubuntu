/* ============================================================================
   Section 21.8 : Algorithmes parallèles - std::transform_reduce
   Description : Map-reduce parallèle (masse salariale des employés actifs)
   Fichier source : 08-algorithmes-paralleles.md
   Note : Sans TBB installé, l'exécution reste séquentielle (pas d'erreur)
   ============================================================================ */

#include <numeric>
#include <execution>
#include <vector>
#include <string>
#include <print>

struct Employee {
    std::string name;
    double salary;
    bool active;
};

int main() {
    std::vector<Employee> employees = {
        {"Alice", 55000.0, true},
        {"Bob", 48000.0, true},
        {"Charlie", 62000.0, false},
        {"Diana", 51000.0, true},
        {"Eve", 59000.0, true},
        {"Frank", 45000.0, false},
    };

    // Map : extraire le salaire si actif (sinon 0)
    // Reduce : sommer
    double total = std::transform_reduce(
        std::execution::par,
        employees.begin(), employees.end(),
        0.0,                                          // Valeur initiale (reduce)
        std::plus<>{},                                // Opération de réduction
        [](const Employee& e) -> double {             // Transformation (map)
            return e.active ? e.salary : 0.0;
        }
    );

    std::println("Masse salariale active : {:.2f}", total);
    // Attendu : 55000 + 48000 + 51000 + 59000 = 213000.00
}
