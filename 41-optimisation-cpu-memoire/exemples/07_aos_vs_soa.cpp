/* ============================================================================
   Section 41.1.3 : Data-oriented design
   Description : Benchmark AoS vs SoA sur une boucle de simulation physique
   Fichier source : 01.3-data-oriented.md
   ============================================================================ */

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

// --- AoS (Array of Structures) ---
struct Particle {
    float x, y, z;
    float vx, vy, vz;
    float r, g, b, a;
    float mass;
    float lifetime;
    std::uint32_t flags;
    std::uint32_t texture_id;
};

void update_positions_aos(std::vector<Particle>& particles, float dt) {
    for (auto& p : particles) {
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.z += p.vz * dt;
    }
}

// --- SoA (Structure of Arrays) ---
struct ParticleSystem {
    std::vector<float> x, y, z;
    std::vector<float> vx, vy, vz;
    std::size_t count{0};

    void resize(std::size_t n) {
        x.resize(n); y.resize(n); z.resize(n);
        vx.resize(n); vy.resize(n); vz.resize(n);
        count = n;
    }
};

void update_positions_soa(ParticleSystem& ps, float dt) {
    for (std::size_t i = 0; i < ps.count; ++i) {
        ps.x[i] += ps.vx[i] * dt;
        ps.y[i] += ps.vy[i] * dt;
        ps.z[i] += ps.vz[i] * dt;
    }
}

int main() {
    constexpr std::size_t N = 100'000;
    constexpr int RUNS = 100;
    float dt = 0.016f;

    // Init AoS
    std::vector<Particle> aos(N);
    for (std::size_t i = 0; i < N; ++i) {
        aos[i] = {0, 0, 0,  1.0f, 0.5f, 0.2f,  1, 1, 1, 1,  1.0f, 10.0f, 0, 0};
    }

    // Init SoA
    ParticleSystem soa;
    soa.resize(N);
    for (std::size_t i = 0; i < N; ++i) {
        soa.vx[i] = 1.0f; soa.vy[i] = 0.5f; soa.vz[i] = 0.2f;
    }

    // Benchmark AoS
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int r = 0; r < RUNS; ++r)
        update_positions_aos(aos, dt);
    auto t2 = std::chrono::high_resolution_clock::now();
    auto us1 = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();

    // Benchmark SoA
    auto t3 = std::chrono::high_resolution_clock::now();
    for (int r = 0; r < RUNS; ++r)
        update_positions_soa(soa, dt);
    auto t4 = std::chrono::high_resolution_clock::now();
    auto us2 = std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count();

    std::cout << "AoS: " << us1 << " us (" << RUNS << " runs)\n";
    std::cout << "SoA: " << us2 << " us (" << RUNS << " runs)\n";
    std::cout << "Speedup SoA/AoS: " << (us2 > 0 ? static_cast<double>(us1) / us2 : 0) << "x\n";
}
