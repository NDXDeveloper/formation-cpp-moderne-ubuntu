/* ============================================================================
   Section 6.2.2 : Constructeur paramétré
   Description : Surcharge de constructeurs (Color), constructeurs délégués
                 (Connection), validation (PortRange), factory functions,
                 DynArray avec constructeur délégué
   Fichier source : 02.2-constructeur-parametre.md
   ============================================================================ */
#include <iostream>
#include <string>
#include <cstdint>
#include <algorithm>
#include <stdexcept>
#include <cstddef>

// --- Color : factory functions ---
class Color {
public:
    static Color from_rgb(uint8_t r, uint8_t g, uint8_t b) {
        return Color(r, g, b);
    }

    static Color from_hex(uint32_t hex) {
        return Color((hex >> 16) & 0xFF,
                     (hex >> 8) & 0xFF,
                     hex & 0xFF);
    }

    static Color from_gray(uint8_t level) {
        return Color(level, level, level);
    }

    uint8_t r() const { return r_; }
    uint8_t g() const { return g_; }
    uint8_t b() const { return b_; }

private:
    Color(uint8_t r, uint8_t g, uint8_t b)
        : r_(r), g_(g), b_(b) {}

    uint8_t r_, g_, b_;
};

// --- Connection : constructeurs délégués ---
class Connection {
public:
    Connection(const std::string& host, uint16_t port, int timeout_ms)
        : host_(host), port_(port), timeout_ms_(timeout_ms) {
        validate();
    }

    Connection(const std::string& host, uint16_t port)
        : Connection(host, port, 5000) {}

    Connection(const std::string& host)
        : Connection(host, 8080) {}

    const std::string& host() const { return host_; }
    uint16_t port() const { return port_; }
    int timeout_ms() const { return timeout_ms_; }

private:
    void validate() {
        if (host_.empty()) throw std::invalid_argument("Host cannot be empty");
        if (port_ == 0) throw std::invalid_argument("Port cannot be 0");
    }

    std::string host_;
    uint16_t port_;
    int timeout_ms_;
};

// --- PortRange : validation dans le constructeur ---
class PortRange {
public:
    PortRange(uint16_t first, uint16_t last)
        : first_(first), last_(last) {
        if (first > last) {
            throw std::invalid_argument(
                "PortRange: first (" + std::to_string(first) +
                ") must be <= last (" + std::to_string(last) + ")"
            );
        }
    }

    uint16_t first() const { return first_; }
    uint16_t last() const { return last_; }
    uint16_t count() const { return last_ - first_ + 1; }

private:
    uint16_t first_;
    uint16_t last_;
};

// --- DynArray : constructeur délégué ---
class DynArray {
public:
    DynArray() = default;

    explicit DynArray(std::size_t size)
        : data_(size > 0 ? new int[size]{} : nullptr)
        , size_(size) {}

    DynArray(std::size_t size, int value)
        : DynArray(size) {
        std::fill_n(data_, size_, value);
    }

    ~DynArray() { delete[] data_; }

    std::size_t size() const { return size_; }

    int& operator[](std::size_t index) {
        if (index >= size_) throw std::out_of_range("out of range");
        return data_[index];
    }

private:
    int* data_ = nullptr;
    std::size_t size_ = 0;
};

int main() {
    // Color factory functions
    Color sky   = Color::from_rgb(135, 206, 235);
    Color brand = Color::from_hex(0xFF5733);
    Color dark  = Color::from_gray(30);
    std::cout << "sky: " << (int)sky.r() << ","
              << (int)sky.g() << "," << (int)sky.b() << "\n";
    std::cout << "brand: " << (int)brand.r() << ","
              << (int)brand.g() << "," << (int)brand.b() << "\n";
    std::cout << "dark: " << (int)dark.r() << ","
              << (int)dark.g() << "," << (int)dark.b() << "\n";

    // Connection délégation
    Connection c1("api.example.com", 443, 10000);
    Connection c2("api.example.com", 443);
    Connection c3("api.example.com");
    std::cout << "c1: " << c1.host() << ":" << c1.port()
              << " t=" << c1.timeout_ms() << "\n";
    std::cout << "c2: " << c2.host() << ":" << c2.port()
              << " t=" << c2.timeout_ms() << "\n";
    std::cout << "c3: " << c3.host() << ":" << c3.port()
              << " t=" << c3.timeout_ms() << "\n";

    // PortRange validation
    PortRange http(80, 80);
    PortRange dynamic(49152, 65535);
    std::cout << "http count: " << http.count() << "\n";
    std::cout << "dynamic count: " << dynamic.count() << "\n";

    try {
        PortRange invalid(100, 50);
    } catch (const std::invalid_argument& e) {
        std::cout << "Exception: " << e.what() << "\n";
    }

    // DynArray délégation
    DynArray zeros(5);
    DynArray nines(3, 9);
    std::cout << "zeros[0]: " << zeros[0] << "\n";
    std::cout << "nines[0]: " << nines[0] << ", nines[2]: " << nines[2] << "\n";

    return 0;
}
// Sortie attendue :
// sky: 135,206,235
// brand: 255,87,51
// dark: 30,30,30
// c1: api.example.com:443 t=10000
// c2: api.example.com:443 t=5000
// c3: api.example.com:8080 t=5000
// http count: 1
// dynamic count: 16384
// Exception: PortRange: first (100) must be <= last (50)
// zeros[0]: 0
// nines[0]: 9, nines[2]: 9
