/* ============================================================================
   Section 46.2 : Separation .h/.cpp et compilation incrementale
   Description : Idiome Pimpl — implementation cachee dans le .cpp
   Fichier source : 02-separation-h-cpp.md
   ============================================================================ */

#include "01_pimpl.h"
#include <atomic>
#include <iostream>

namespace monprojet::core {

struct Engine::Impl {
    std::string config_name;
    std::atomic<bool> running{false};
    std::atomic<std::uint64_t> counter{0};
};

Engine::Engine(std::string config_name)
    : impl_(std::make_unique<Impl>()) {
    impl_->config_name = std::move(config_name);
}

Engine::~Engine() = default;
Engine::Engine(Engine&&) noexcept = default;
Engine& Engine::operator=(Engine&&) noexcept = default;

bool Engine::start() {
    impl_->running = true;
    impl_->counter = 0;
    return true;
}

void Engine::stop() {
    impl_->running = false;
}

std::uint64_t Engine::processed_count() const {
    return impl_->counter.load();
}

} // namespace monprojet::core
