/* ============================================================================
   Section 7.3 : Polymorphisme dynamique — Pattern complet
   Description : Exemple complet avec virtual, override et final.
                 Pipeline de taches polymorphiques avec unique_ptr,
                 dispatch dynamique, classe final et methode avec
                 implementation par defaut.
   Fichier source : 03-polymorphisme-dynamique.md
   ============================================================================ */
#include <memory>
#include <vector>
#include <print>
#include <format>
#include <string>

class Tache {
    std::string nom_;
public:
    explicit Tache(std::string nom) : nom_{std::move(nom)} {}

    virtual void executer() = 0;

    virtual std::string description() const {
        return nom_;
    }

    std::string const& nom() const { return nom_; }

    virtual ~Tache() = default;
};

class TacheSysteme : public Tache {
    std::string commande_;
public:
    TacheSysteme(std::string nom, std::string cmd)
        : Tache{std::move(nom)}, commande_{std::move(cmd)} {}

    void executer() override {
        std::println("Exécution système : {}", commande_);
    }

    std::string description() const override {
        return std::format("{} [cmd: {}]", nom(), commande_);
    }
};

class TacheLog final : public Tache {
    std::string message_;
public:
    TacheLog(std::string nom, std::string message)
        : Tache{std::move(nom)}, message_{std::move(message)} {}

    void executer() override {
        std::println("[LOG] {}", message_);
    }

    // description() n'est pas redéfinie → utilise Tache::description()
};

void executer_pipeline(std::vector<std::unique_ptr<Tache>>& pipeline) {
    for (auto const& tache : pipeline) {
        std::println("--- {} ---", tache->description());
        tache->executer();    // dispatch dynamique → bonne implémentation
    }
}

int main() {
    std::vector<std::unique_ptr<Tache>> pipeline;
    pipeline.push_back(std::make_unique<TacheSysteme>("Backup", "tar czf backup.tar.gz /data"));
    pipeline.push_back(std::make_unique<TacheLog>("Notification", "Pipeline démarré"));
    pipeline.push_back(std::make_unique<TacheSysteme>("Deploy", "kubectl apply -f deploy.yaml"));

    executer_pipeline(pipeline);
}
