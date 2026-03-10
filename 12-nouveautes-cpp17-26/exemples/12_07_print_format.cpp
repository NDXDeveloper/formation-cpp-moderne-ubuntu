/* ============================================================================
   Section 12.7 : std::print et std::format (C++23)
   Description : Formatage moderne - format basique, print/println,
                 alignement, entiers, flottants, chaines, booleans,
                 chrono, formatter personnalise, format_to, formatted_size
   Fichier source : 07-std-print-format.md
   ============================================================================ */
#include <format>
#include <print>
#include <string>
#include <vector>
#include <iterator>
#include <chrono>
#include <cstdint>

// === Custom formatter pour Point (lignes 238-254) ===
struct Point {
    double x, y;
};

template <>
struct std::formatter<Point> {
    char mode = 'c';

    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && (*it == 'c' || *it == 'v')) {
            mode = *it;
            ++it;
        }
        return it;
    }

    auto format(const Point& p, std::format_context& ctx) const {
        if (mode == 'v') {
            return std::format_to(ctx.out(), "Point(x={:.2f}, y={:.2f})", p.x, p.y);
        }
        return std::format_to(ctx.out(), "({:.2f}, {:.2f})", p.x, p.y);
    }
};

// === UserId formatter heritant de uint64_t (lignes 308-318) ===
struct UserId {
    uint64_t value;
};

template <>
struct std::formatter<UserId> : std::formatter<uint64_t> {
    auto format(const UserId& id, std::format_context& ctx) const {
        return std::formatter<uint64_t>::format(id.value, ctx);
    }
};

int main() {
    // === std::format basique (lignes 22-26) ===
    std::print("=== format basique ===\n");
    std::string msg = std::format("Bonjour, {} ! Vous avez {} messages.", "Alice", 42);
    std::print("{}\n", msg);

    // === std::print et println (lignes 47-53) ===
    std::print("\n=== print/println ===\n");
    std::print("Temperature : {:.1f} C\n", 22.5);
    std::println("Ligne avec retour a la ligne automatique : {}", 42);

    // === Indexation (lignes 101-103) ===
    std::print("\n=== indexation ===\n");
    std::print("{0} a {1} ans. {0} habite a Paris.\n", "Alice", 30);

    // === Alignement (lignes 113-121) ===
    std::print("\n=== alignement ===\n");
    std::print("[{:<10}]\n", "hello");
    std::print("[{:>10}]\n", "hello");
    std::print("[{:^10}]\n", "hello");
    std::print("[{:*<10}]\n", "hello");
    std::print("[{:*>10}]\n", "hello");
    std::print("[{:-^20}]\n", "TITRE");

    // Largeur dynamique
    int width = 15;
    std::print("[{:>{}}]\n", "hello", width);

    // === Entiers (lignes 134-151) ===
    std::print("\n=== entiers ===\n");
    std::print("{:d}\n", 255);
    std::print("{:b}\n", 255);
    std::print("{:o}\n", 255);
    std::print("{:x}\n", 255);
    std::print("{:X}\n", 255);
    std::print("{:#b}\n", 255);
    std::print("{:#o}\n", 255);
    std::print("{:#x}\n", 255);
    std::print("{:08d}\n", 42);
    std::print("{:08x}\n", 255);

    // === Flottants (lignes 157-179) ===
    std::print("\n=== flottants ===\n");
    std::print("{}\n", 3.14159);
    std::print("{:f}\n", 3.14159);
    std::print("{:.2f}\n", 3.14159);
    std::print("{:e}\n", 3.14159);
    std::print("{:.3e}\n", 0.000042);
    std::print("{:g}\n", 3.14159);
    std::print("{:g}\n", 0.000042);
    std::print("[{:10.2f}]\n", 3.14159);
    std::print("[{:010.2f}]\n", 3.14159);
    std::print("{:+.2f}\n", 3.14);
    std::print("{:+.2f}\n", -3.14);
    std::print("{: .2f}\n", 3.14);

    // === Chaines (lignes 185-192) ===
    std::print("\n=== chaines ===\n");
    std::print("{:.5}\n", "Hello, World!");
    std::print("[{:10.5}]\n", "Hello, World!");
    std::print("{:?}\n", "Hello\tWorld\n");

    // === Booleens (lignes 200-202) ===
    std::print("\n=== booleens ===\n");
    std::print("{}\n", true);
    std::print("{}\n", false);

    // === Chrono (lignes 214-223) ===
    std::print("\n=== chrono ===\n");
    auto duration = std::chrono::hours(2) + std::chrono::minutes(30);
    std::print("Duree : {:%H:%M}\n", duration);

    // === Custom Point formatter (lignes 260-300) ===
    std::print("\n=== custom formatter Point ===\n");
    Point origin{0.0, 0.0};
    Point target{3.14, 2.72};
    std::print("De {} vers {}\n", origin, target);
    std::string s = std::format("Position : {}", target);
    std::print("{}\n", s);

    Point p{3.14, 2.72};
    std::print("{:c}\n", p);
    std::print("{:v}\n", p);
    std::print("{}\n", p);

    // === UserId formatter (lignes 321-322) ===
    std::print("\n=== UserId formatter ===\n");
    UserId user{12345};
    std::print("User #{:08x}\n", user);

    // === format_to (lignes 336-349) ===
    std::print("\n=== format_to ===\n");
    std::vector<char> buffer;
    std::format_to(std::back_inserter(buffer), "x={}, y={}", 10, 20);
    std::print("buffer: {}\n", std::string_view(buffer.data(), buffer.size()));

    std::string log;
    std::format_to(std::back_inserter(log), "[INFO] {}\n", "Demarrage");
    std::format_to(std::back_inserter(log), "[INFO] {}\n", "Pret");
    std::print("log:\n{}", log);

    char fixed_buf[64];
    auto result = std::format_to_n(fixed_buf, sizeof(fixed_buf) - 1, "Score: {}", 42);
    *result.out = '\0';
    std::print("fixed_buf: {}\n", fixed_buf);

    // === formatted_size (lignes 358-360) ===
    std::print("\n=== formatted_size ===\n");
    auto size = std::formatted_size("Le résultat est {:.4f}", 3.14159);
    std::print("formatted_size = {}\n", size);
    // size == 23
}
