/* ============================================================================
   Section 46.2 : Separation .h/.cpp et compilation incrementale
   Description : Idiome Pimpl — header public avec pointeur opaque
   Fichier source : 02-separation-h-cpp.md
   ============================================================================ */

#pragma once
#include <memory>
#include <string>
#include <cstdint>

namespace monprojet::core {

class Engine {
public:
    explicit Engine(std::string config_name);
    ~Engine();

    Engine(Engine&&) noexcept;
    Engine& operator=(Engine&&) noexcept;

    [[nodiscard]] bool start();
    void stop();
    [[nodiscard]] std::uint64_t processed_count() const;

private:
    struct Impl;                   // Forward declaration
    std::unique_ptr<Impl> impl_;   // Pointeur opaque
};

} // namespace monprojet::core
