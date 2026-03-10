/* ============================================================================
   Section 3.5.3 : consteval - Fonctions obligatoirement compile-time
   Description : CRC table, string hash avec switch, checked_port, rgba,
                 if consteval, consteval appelant constexpr
   Fichier source : 05.3-consteval.md
   ============================================================================ */
#include <print>
#include <array>
#include <cstdint>
#include <cmath>

// --- square consteval (lignes 23-25) ---
consteval int square(int n) {
    return n * n;
}

// --- Comparaison constexpr vs consteval (lignes 37-38) ---
constexpr int square_cx(int n) { return n * n; }
consteval int square_ce(int n) { return n * n; }

// --- CRC table (lignes 73-86) ---
consteval std::array<uint8_t, 256> generate_crc_table() {
    std::array<uint8_t, 256> table{};
    for (int i = 0; i < 256; ++i) {
        uint8_t crc = static_cast<uint8_t>(i);
        for (int j = 0; j < 8; ++j) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xA7;
            else
                crc >>= 1;
        }
        table[i] = crc;
    }
    return table;
}

// --- String hash FNV-1a (lignes 98-105) ---
consteval uint32_t string_hash(const char* str) {
    uint32_t hash = 2166136261u;
    while (*str) {
        hash ^= static_cast<uint32_t>(*str++);
        hash *= 16777619u;
    }
    return hash;
}

// --- Validation port (lignes 131-136) ---
consteval int checked_port(int port) {
    if (port < 0 || port > 65535) {
        throw "Port hors de la plage valide [0, 65535]";
    }
    return port;
}

// --- RGBA (lignes 148-153) ---
consteval uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    return (static_cast<uint32_t>(a) << 24)
         | (static_cast<uint32_t>(r) << 16)
         | (static_cast<uint32_t>(g) << 8)
         | static_cast<uint32_t>(b);
}

// --- if consteval (lignes 171-184) ---
constexpr double fast_sqrt(double x) {
    if consteval {
        double guess = x / 2.0;
        for (int i = 0; i < 100; ++i) {
            guess = (guess + x / guess) / 2.0;
        }
        return guess;
    } else {
        return std::sqrt(x);
    }
}

// --- consteval appelant constexpr (lignes 238-244) ---
constexpr int double_it(int n) { return n * 2; }
consteval int quadruple(int n) {
    return double_it(double_it(n));
}

int main() {
    // --- Comparaison constexpr vs consteval (lignes 41-42) ---
    constexpr int a = square_cx(5);
    constexpr int b = square_ce(5);
    std::print("square_cx(5)={}, square_ce(5)={}\n", a, b);

    // --- CRC table (ligne 88) ---
    constexpr auto crc_table = generate_crc_table();
    std::print("crc_table[0]={}, crc_table[1]={}, crc_table[255]={}\n",
               crc_table[0], crc_table[1], crc_table[255]);

    // --- String hash dans un switch (lignes 108-121) ---
    constexpr auto EVENT_CLICK    = string_hash("mouse_click");
    constexpr auto EVENT_KEYDOWN  = string_hash("key_down");
    constexpr auto EVENT_RESIZE   = string_hash("window_resize");
    std::print("CLICK=0x{:08X}, KEYDOWN=0x{:08X}, RESIZE=0x{:08X}\n",
               EVENT_CLICK, EVENT_KEYDOWN, EVENT_RESIZE);

    uint32_t event_id = string_hash("mouse_click");
    switch (event_id) {
        case string_hash("mouse_click"):
            std::print("Click event matched!\n");
            break;
        case string_hash("key_down"):
            std::print("Key down event matched!\n");
            break;
    }

    // --- checked_port (lignes 138-140) ---
    constexpr auto http_port = checked_port(80);
    constexpr auto https_port = checked_port(443);
    std::print("http={}, https={}\n", http_port, https_port);

    // --- RGBA (lignes 156-159) ---
    constexpr auto COLOR_RED     = rgba(255, 0, 0);
    constexpr auto COLOR_GREEN   = rgba(0, 255, 0);
    constexpr auto COLOR_BLUE    = rgba(0, 0, 255);
    constexpr auto COLOR_OVERLAY = rgba(0, 0, 0, 128);
    std::print("RED=0x{:08X}, GREEN=0x{:08X}, BLUE=0x{:08X}, OVERLAY=0x{:08X}\n",
               COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_OVERLAY);

    // --- if consteval (lignes 186-187) ---
    constexpr double compile_time_result = fast_sqrt(2.0);
    std::print("fast_sqrt(2.0) compile-time = {:.15f}\n", compile_time_result);
    double runtime_val = 2.0;
    double runtime_result = fast_sqrt(runtime_val);
    std::print("fast_sqrt(2.0) runtime = {:.15f}\n", runtime_result);

    // --- quadruple (ligne 244) ---
    constexpr int result = quadruple(3);
    std::print("quadruple(3)={}\n", result);
    static_assert(result == 12);

    return 0;
}
