/* ============================================================================
   Section 42.2 : Manipulation de Bits et Bitfields
   Description : Pattern enum class + EnableBitmask pour flags type-safe
   Fichier source : 02-manipulation-bits.md
   ============================================================================ */

#include <cstdint>
#include <iostream>
#include <type_traits>

// Trait pour activer les operateurs bit a bit sur un enum
template <typename E>
struct EnableBitmask : std::false_type {};

template <typename E>
    requires EnableBitmask<E>::value
constexpr E operator|(E lhs, E rhs) {
    using U = std::underlying_type_t<E>;
    return static_cast<E>(static_cast<U>(lhs) | static_cast<U>(rhs));
}

template <typename E>
    requires EnableBitmask<E>::value
constexpr E operator&(E lhs, E rhs) {
    using U = std::underlying_type_t<E>;
    return static_cast<E>(static_cast<U>(lhs) & static_cast<U>(rhs));
}

template <typename E>
    requires EnableBitmask<E>::value
constexpr E operator~(E val) {
    using U = std::underlying_type_t<E>;
    return static_cast<E>(~static_cast<U>(val));
}

template <typename E>
    requires EnableBitmask<E>::value
constexpr E& operator|=(E& lhs, E rhs) { return lhs = lhs | rhs; }

template <typename E>
    requires EnableBitmask<E>::value
constexpr E& operator&=(E& lhs, E rhs) { return lhs = lhs & rhs; }

// Definition de l'enum
enum class FilePermission : uint8_t {
    None    = 0,
    Read    = 1 << 0,   // 0b001
    Write   = 1 << 1,   // 0b010
    Execute = 1 << 2,   // 0b100
};

// Active les operateurs bit a bit
template <>
struct EnableBitmask<FilePermission> : std::true_type {};

int main() {
    FilePermission perms = FilePermission::Read | FilePermission::Write;

    bool can_read  = (perms & FilePermission::Read) != FilePermission::None;
    bool can_exec  = (perms & FilePermission::Execute) != FilePermission::None;

    std::cout << "Read:    " << can_read << "\n";   // 1
    std::cout << "Execute: " << can_exec << "\n";   // 0

    perms |= FilePermission::Execute;
    can_exec = (perms & FilePermission::Execute) != FilePermission::None;
    std::cout << "After |= Execute: " << can_exec << "\n";  // 1

    perms &= ~FilePermission::Write;
    bool can_write = (perms & FilePermission::Write) != FilePermission::None;
    std::cout << "After &= ~Write: " << can_write << "\n";  // 0
}
