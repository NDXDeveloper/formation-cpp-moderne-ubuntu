/* ============================================================================
   Section 9.1.3 : Custom deleters
   Description : Lambda, pointeur de fonction, foncteur, tailles, factory RAII
   Fichier source : 01.3-custom-deleters.md
   ============================================================================ */
#include <memory>
#include <print>
#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>
#include <fcntl.h>

// === Cas n°1 : FILE* avec lambda (lignes 56-79) ===
void exemple_file_lambda() {
    std::print("=== FILE* avec lambda ===\n");

    auto deleter = [](FILE* f) {
        if (f) fclose(f);
    };

    std::unique_ptr<FILE, decltype(deleter)> fichier(
        fopen("/tmp/test_custom_deleter.txt", "w"),
        deleter
    );

    if (!fichier) {
        std::print("Impossible d'ouvrir le fichier\n");
        return;
    }

    std::fputs("Hello custom deleter!\n", fichier.get());
    std::print("Fichier écrit avec succès\n");
    // fclose() appelé automatiquement
}

// === Lambda C++20 dans le type (lignes 102-111) ===
void exemple_cpp20_lambda() {
    std::print("\n=== C++20 lambda dans le type ===\n");

    using FilePtr = std::unique_ptr<FILE, decltype([](FILE* f) { if (f) fclose(f); })>;

    auto ouvrir = [](const char* chemin, const char* mode) -> FilePtr {
        return FilePtr(fopen(chemin, mode));
    };

    auto f = ouvrir("/tmp/test_custom_deleter.txt", "r");
    if (f) {
        char buf[256];
        if (fgets(buf, sizeof(buf), f.get())) {
            std::print("Lu: {}", buf);
        }
    }
}

// === Pointeur de fonction (lignes 122-126) ===
void exemple_ptr_fonction() {
    std::print("\n=== Pointeur de fonction ===\n");

    std::unique_ptr<FILE, int(*)(FILE*)> fichier(
        fopen("/tmp/test_custom_deleter.txt", "r"),
        fclose
    );

    if (fichier) {
        char buf[256];
        if (fgets(buf, sizeof(buf), fichier.get())) {
            std::print("Lu via ptr fonction: {}", buf);
        }
    }
}

// === Foncteur (lignes 148-163) ===
struct FermerFichier {
    void operator()(FILE* f) const noexcept {
        if (f) {
            std::print("[debug] Fermeture du fichier\n");
            fclose(f);
        }
    }
};

using FilePtr = std::unique_ptr<FILE, FermerFichier>;

FilePtr ouvrir_fichier(const char* chemin, const char* mode) {
    return FilePtr(fopen(chemin, mode));
}

void exemple_foncteur() {
    std::print("\n=== Foncteur ===\n");
    auto f = ouvrir_fichier("/tmp/test_custom_deleter.txt", "r");
    if (f) {
        char buf[256];
        if (fgets(buf, sizeof(buf), f.get())) {
            std::print("Lu via foncteur: {}", buf);
        }
    }
    // "[debug] Fermeture du fichier" affiché automatiquement
}

// === Comparaison des tailles (lignes 131-141) ===
void exemple_tailles() {
    std::print("\n=== Comparaison des tailles ===\n");

    std::print("Deleter par défaut : {}\n",
        sizeof(std::unique_ptr<FILE>));                    // 8

    std::print("Pointeur de fonction : {}\n",
        sizeof(std::unique_ptr<FILE, int(*)(FILE*)>));     // 16

    std::print("Lambda sans capture : {}\n",
        sizeof(std::unique_ptr<FILE,
            decltype([](FILE* f){ if (f) fclose(f); })>));  // 8

    std::print("Foncteur sans membre : {}\n",
        sizeof(FilePtr));                                    // 8
}

// === FileDescriptor RAII (lignes 184-207) ===
class FileDescriptor {
    int fd_ = -1;
public:
    explicit FileDescriptor(int fd) : fd_(fd) {}
    ~FileDescriptor() { if (fd_ >= 0) ::close(fd_); }

    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;

    FileDescriptor(FileDescriptor&& other) noexcept
        : fd_(other.fd_) { other.fd_ = -1; }
    FileDescriptor& operator=(FileDescriptor&& other) noexcept {
        if (this != &other) {
            if (fd_ >= 0) ::close(fd_);
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    int get() const noexcept { return fd_; }
    explicit operator bool() const noexcept { return fd_ >= 0; }
};

void exemple_fd() {
    std::print("\n=== FileDescriptor RAII ===\n");

    FileDescriptor fd(::open("/tmp/test_custom_deleter.txt", O_RDONLY));
    if (fd) {
        char buf[256];
        auto n = ::read(fd.get(), buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            std::print("Lu via fd: {}", buf);
        }
    }
    // close() appelé automatiquement
}

// === malloc/free (lignes 299-316) ===
void exemple_malloc() {
    std::print("\n=== malloc/free ===\n");

    auto deleter = [](char* p) { std::free(p); };
    std::unique_ptr<char, decltype(deleter)> data(
        static_cast<char*>(std::malloc(64)),
        deleter
    );

    if (data) {
        std::strcpy(data.get(), "Alloué par malloc");
        std::print("Données : {}\n", data.get());
    }
    // free() appelé automatiquement
}

// === Factory générique (lignes 386-435) ===
struct FCloser {
    void operator()(FILE* f) const noexcept { if (f) fclose(f); }
};
using UniqueFile = std::unique_ptr<FILE, FCloser>;

inline UniqueFile ouvrir(const char* chemin, const char* mode) {
    return UniqueFile(fopen(chemin, mode));
}

struct PCloser {
    void operator()(FILE* f) const noexcept { if (f) pclose(f); }
};
using UniquePipe = std::unique_ptr<FILE, PCloser>;

inline UniquePipe ouvrir_pipe(const char* cmd, const char* mode) {
    return UniquePipe(popen(cmd, mode));
}

void exemple_factory() {
    std::print("\n=== Factory générique ===\n");

    auto fichier = ouvrir("/etc/hostname", "r");
    if (fichier) {
        char buf[256];
        if (fgets(buf, sizeof(buf), fichier.get())) {
            std::print("Hostname : {}", buf);
        }
    }

    auto pipe = ouvrir_pipe("echo 'Hello depuis pipe'", "r");
    if (pipe) {
        char buf[256];
        if (fgets(buf, sizeof(buf), pipe.get())) {
            std::print("{}", buf);
        }
    }
}

int main() {
    exemple_file_lambda();
    exemple_cpp20_lambda();
    exemple_ptr_fonction();
    exemple_foncteur();
    exemple_tailles();
    exemple_fd();
    exemple_malloc();
    exemple_factory();
    std::print("\n✅ Tous les exemples passés\n");
}
