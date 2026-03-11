/* ============================================================================
   Section 17.3 : Exceptions personnalisées
   Description : Conception de classes d'exceptions métier — héritage de
                 std::runtime_error, transport d'information contextuelle,
                 hiérarchie d'exceptions, std::throw_with_nested,
                 surcharge de what(), std::source_location
   Fichier source : 03-exceptions-personnalisees.md
   ============================================================================ */

#include <stdexcept>
#include <print>
#include <string>
#include <cstdint>
#include <exception>
#include <source_location>
#include <fstream>

// === Exception simple : FichierIntrouvableError ===
class FichierIntrouvableError : public std::runtime_error {
public:
    explicit FichierIntrouvableError(const std::string& chemin)
        : std::runtime_error("Fichier introuvable : " + chemin)
        , chemin_(chemin)
    {}
    const std::string& chemin() const noexcept { return chemin_; }
private:
    std::string chemin_;
};

// === ParseError avec localisation ===
class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& message,
               std::size_t ligne,
               std::size_t colonne,
               const std::string& fichier = "")
        : std::runtime_error(formater_message(message, ligne, colonne, fichier))
        , ligne_(ligne), colonne_(colonne), fichier_(fichier)
    {}
    std::size_t ligne()   const noexcept { return ligne_; }
    std::size_t colonne() const noexcept { return colonne_; }
    const std::string& fichier() const noexcept { return fichier_; }
private:
    std::size_t ligne_;
    std::size_t colonne_;
    std::string fichier_;

    static std::string formater_message(const std::string& msg,
                                        std::size_t ligne,
                                        std::size_t colonne,
                                        const std::string& fichier) {
        std::string resultat;
        if (!fichier.empty()) resultat += fichier + ":";
        resultat += std::to_string(ligne) + ":" + std::to_string(colonne);
        resultat += " — " + msg;
        return resultat;
    }
};

// === NetworkError avec code et retry ===
class NetworkError : public std::runtime_error {
public:
    enum class Code : std::uint16_t {
        timeout          = 1,
        connexion_refusee = 2,
        dns_non_resolu   = 3,
        tls_invalide     = 4
    };

    NetworkError(Code code, const std::string& hote, std::uint16_t port,
                 const std::string& detail = "")
        : std::runtime_error(formater(code, hote, port, detail))
        , code_(code), hote_(hote), port_(port)
    {}

    Code               code() const noexcept { return code_; }
    const std::string& hote() const noexcept { return hote_; }
    std::uint16_t      port() const noexcept { return port_; }
    bool est_transitoire() const noexcept {
        return code_ == Code::timeout || code_ == Code::connexion_refusee;
    }

private:
    Code          code_;
    std::string   hote_;
    std::uint16_t port_;

    static std::string formater(Code code, const std::string& hote,
                                std::uint16_t port, const std::string& detail) {
        std::string msg = "Erreur réseau [" + std::to_string(static_cast<int>(code)) + "] "
                        + "vers " + hote + ":" + std::to_string(port);
        if (!detail.empty()) msg += " — " + detail;
        return msg;
    }
};

// === Hiérarchie d'exceptions applicative ===
class AppError : public std::runtime_error {
public:
    enum class Domaine : int {
        general    = 0,
        database   = 1000,
        reseau     = 2000,
        validation = 3000
    };

    AppError(Domaine domaine, int code_detail, const std::string& message)
        : std::runtime_error(message)
        , domaine_(domaine), code_detail_(code_detail)
    {}

    Domaine domaine()     const noexcept { return domaine_; }
    int     code_detail() const noexcept { return code_detail_; }
    int code_complet() const noexcept {
        return static_cast<int>(domaine_) + code_detail_;
    }

private:
    Domaine domaine_;
    int     code_detail_;
};

class DatabaseError : public AppError {
public:
    explicit DatabaseError(int code_detail, const std::string& message,
                           const std::string& requete = "")
        : AppError(Domaine::database, code_detail, message)
        , requete_(requete)
    {}
    const std::string& requete() const noexcept { return requete_; }
private:
    std::string requete_;
};

class ConnectionError : public DatabaseError {
public:
    explicit ConnectionError(const std::string& hote, const std::string& detail = "")
        : DatabaseError(1, "Connexion DB échouée vers " + hote +
                           (detail.empty() ? "" : " — " + detail))
        , hote_(hote)
    {}
    const std::string& hote() const noexcept { return hote_; }
private:
    std::string hote_;
};

// === Exception sans allocation (what() surchargé) ===
class ErreurCode : public std::exception {
public:
    explicit ErreurCode(int code) noexcept : code_(code) {
        std::snprintf(buffer_, sizeof(buffer_), "Erreur système (code %d)", code_);
    }
    const char* what() const noexcept override { return buffer_; }
    int code() const noexcept { return code_; }
private:
    int  code_;
    char buffer_[64];
};

// === Exception avec std::source_location ===
class ServiceError : public std::runtime_error {
public:
    enum class Categorie { database, reseau, validation, autorisation };
    ServiceError(Categorie cat, int code, const std::string& message,
                 std::source_location loc = std::source_location::current())
        : std::runtime_error(message)
        , categorie_(cat), code_(code), localisation_(loc)
    {}
    Categorie            categorie()    const noexcept { return categorie_; }
    int                  code()         const noexcept { return code_; }
    std::source_location localisation() const noexcept { return localisation_; }
private:
    Categorie            categorie_;
    int                  code_;
    std::source_location localisation_;
};

// === Nested exceptions ===
void afficher_chaine_erreurs(const std::exception& e, int niveau = 0) {
    std::string indentation(niveau * 2, ' ');
    std::print("{}Causé par : {}\n", indentation, e.what());
    try {
        std::rethrow_if_nested(e);
    } catch (const std::exception& cause) {
        afficher_chaine_erreurs(cause, niveau + 1);
    } catch (...) {
        std::print("{}Causé par : (exception inconnue)\n", indentation);
    }
}

int main() {
    std::print("=== 1. FichierIntrouvableError ===\n");
    try {
        throw FichierIntrouvableError("/etc/monapp/config.yaml");
    } catch (const FichierIntrouvableError& e) {
        std::print("  {} (chemin: {})\n", e.what(), e.chemin());
    }

    std::print("\n=== 2. ParseError avec localisation ===\n");
    try {
        throw ParseError("Clé dupliquée \"port\"", 12, 5, "config.yaml");
    } catch (const ParseError& e) {
        std::print("  {} (ligne:{}, col:{})\n", e.what(), e.ligne(), e.colonne());
    }

    std::print("\n=== 3. NetworkError ===\n");
    try {
        throw NetworkError(NetworkError::Code::timeout, "api.example.com", 443, "délai expiré");
    } catch (const NetworkError& e) {
        std::print("  {} (transitoire: {})\n", e.what(), e.est_transitoire() ? "oui" : "non");
    }

    std::print("\n=== 4. Hiérarchie AppError ===\n");
    try {
        throw ConnectionError("db.prod.local", "connection refused");
    }
    catch (const ConnectionError& e) {
        std::print("  ConnectionError: {} (code: {}, hote: {})\n",
                   e.what(), e.code_complet(), e.hote());
    }
    catch (const DatabaseError& e) {
        std::print("  DatabaseError: {}\n", e.what());
    }
    catch (const AppError& e) {
        std::print("  AppError: {}\n", e.what());
    }

    std::print("\n=== 5. ErreurCode (what() surchargé, sans allocation) ===\n");
    try {
        throw ErreurCode(42);
    } catch (const ErreurCode& e) {
        std::print("  {} (code: {})\n", e.what(), e.code());
    }

    std::print("\n=== 6. ServiceError avec source_location ===\n");
    try {
        throw ServiceError(ServiceError::Categorie::reseau, 2001, "timeout réseau");
    } catch (const ServiceError& e) {
        std::print("  {} (fichier: {}, ligne: {})\n",
                   e.what(), e.localisation().file_name(), e.localisation().line());
    }

    std::print("\n=== 7. Nested exceptions (throw_with_nested) ===\n");
    try {
        try {
            throw std::system_error(
                std::make_error_code(std::errc::connection_refused),
                "Connection refused [errno 111]");
        } catch (const std::system_error&) {
            std::throw_with_nested(
                ConnectionError("db.prod.local", "Échec connexion POSIX"));
        }
    } catch (const std::exception& e) {
        afficher_chaine_erreurs(e);
    }

    std::print("\nProgramme terminé.\n");
    return 0;
}
