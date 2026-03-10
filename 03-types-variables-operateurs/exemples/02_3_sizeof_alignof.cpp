/* ============================================================================
   Section 3.2.3 : sizeof et alignof - Taille et alignement
   Description : Padding, layout memoire, alignas, offsetof,
                 [[no_unique_address]], optimisation de structures
   Fichier source : 02.3-sizeof-alignof.md
   ============================================================================ */
#include <print>
#include <cstddef>
#include <cstdint>
#include <iterator>

int main() {
    // --- sizeof sur les tableaux (lignes 58-63) ---
    int arr[10];
    std::print("Taille totale : {} octets\n", sizeof(arr));      // 40
    std::print("Taille element : {} octets\n", sizeof(arr[0]));  // 4
    std::print("Nombre elements : {}\n", sizeof(arr) / sizeof(arr[0])); // 10
    std::print("std::size(arr) : {}\n", std::size(arr));          // 10

    // --- Point simple (lignes 86-91) ---
    struct Point {
        double x; // 8
        double y; // 8
    };
    std::print("sizeof(Point) = {}\n", sizeof(Point)); // 16
    static_assert(sizeof(Point) == 16);

    // --- Mixed avec padding (lignes 147-153) ---
    struct Mixed {
        char   c;   // 1 + 3 padding
        int    i;   // 4
        double d;   // 8
    };
    std::print("sizeof(Mixed)  = {}\n", sizeof(Mixed));  // 16
    std::print("alignof(Mixed) = {}\n", alignof(Mixed)); // 8
    static_assert(sizeof(Mixed) == 16);
    static_assert(alignof(Mixed) == 8);

    // --- Tail padding (lignes 172-178) ---
    struct Tail {
        double d;  // 8
        char   c;  // 1 + 7 padding
    };
    std::print("sizeof(Tail) = {}\n", sizeof(Tail)); // 16
    static_assert(sizeof(Tail) == 16);

    // --- BadLayout vs GoodLayout (lignes 194-216) ---
    struct BadLayout {
        char   a;   // 1 + 7 padding
        double b;   // 8
        char   c;   // 1 + 3 padding
        int    d;   // 4
        char   e;   // 1 + 7 padding
    };
    struct GoodLayout {
        double b;   // 8
        int    d;   // 4
        char   a;   // 1
        char   c;   // 1
        char   e;   // 1 + 1 padding
    };
    std::print("sizeof(BadLayout)  = {}\n", sizeof(BadLayout));  // 32
    std::print("sizeof(GoodLayout) = {}\n", sizeof(GoodLayout)); // 16
    static_assert(sizeof(BadLayout) == 32);
    static_assert(sizeof(GoodLayout) == 16);

    // --- alignas (lignes 230-235) ---
    struct alignas(64) CacheLine {
        int data[16]; // 64 octets
    };
    std::print("sizeof(CacheLine)  = {}\n", sizeof(CacheLine));  // 64
    std::print("alignof(CacheLine) = {}\n", alignof(CacheLine)); // 64
    static_assert(sizeof(CacheLine) == 64);
    static_assert(alignof(CacheLine) == 64);

    // --- NetworkPacket avec offsetof (lignes 261-279) ---
    struct NetworkPacket {
        uint32_t source_ip;
        uint32_t dest_ip;
        uint16_t source_port;
        uint16_t dest_port;
        uint8_t  protocol;
        uint8_t  flags;
        uint16_t length;
    };
    static_assert(sizeof(NetworkPacket) == 16, "Layout inattendu");
    static_assert(offsetof(NetworkPacket, source_ip)   == 0);
    static_assert(offsetof(NetworkPacket, dest_ip)     == 4);
    static_assert(offsetof(NetworkPacket, source_port) == 8);
    static_assert(offsetof(NetworkPacket, dest_port)   == 10);
    static_assert(offsetof(NetworkPacket, protocol)    == 12);
    static_assert(offsetof(NetworkPacket, flags)       == 13);
    static_assert(offsetof(NetworkPacket, length)      == 14);
    std::print("sizeof(NetworkPacket) = {} (layout verifie)\n", sizeof(NetworkPacket));

    // --- [[no_unique_address]] (lignes 293-306) ---
    struct Empty {};
    struct Before {
        Empty  tag;
        int    value;
    };
    struct After {
        [[no_unique_address]] Empty tag;
        int value;
    };
    std::print("sizeof(Before) = {}\n", sizeof(Before)); // 8
    std::print("sizeof(After)  = {}\n", sizeof(After));  // 4
    static_assert(sizeof(Before) == 8);
    static_assert(sizeof(After) == 4);

    // --- Particle_Bad vs Particle_Good (lignes 319-335) ---
    struct Particle_Bad {
        char   type;     // 1 + 7 padding
        double x;        // 8
        char   active;   // 1 + 3 padding
        float  mass;     // 4
        char   group;    // 1 + 7 padding
    };
    struct Particle_Good {
        double x;        // 8
        float  mass;     // 4
        char   type;     // 1
        char   active;   // 1
        char   group;    // 1 + 1 padding
    };
    std::print("sizeof(Particle_Bad)  = {}\n", sizeof(Particle_Bad));  // 32
    std::print("sizeof(Particle_Good) = {}\n", sizeof(Particle_Good)); // 16
    static_assert(sizeof(Particle_Bad) == 32);
    static_assert(sizeof(Particle_Good) == 16);

    return 0;
}
