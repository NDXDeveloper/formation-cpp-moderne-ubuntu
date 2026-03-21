/* ============================================================================
   Section 44.2 : Observer, Strategy, Command
   Description : Observer avec Signal<Args...>, std::function et tokens
   Fichier source : 02-behavioral-patterns.md
   ============================================================================ */

#include <functional>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <print>

template<typename... Args>
class Signal {
public:
    using Slot   = std::function<void(Args...)>;
    using SlotId = uint64_t;

    SlotId connect(Slot slot) {
        auto id = next_id_++;
        slots_.push_back({id, std::move(slot)});
        return id;
    }

    void disconnect(SlotId id) {
        std::erase_if(slots_, [id](const auto& e) { return e.id == id; });
    }

    void emit(Args... args) const {
        for (const auto& [id, slot] : slots_)
            slot(args...);
    }

    std::size_t size() const { return slots_.size(); }

private:
    struct Entry { SlotId id; Slot slot; };
    std::vector<Entry> slots_;
    SlotId next_id_ = 0;
};

class TemperatureSensor {
public:
    Signal<float> on_temperature_changed;

    void set_temperature(float temp) {
        if (temp != temperature_) {
            temperature_ = temp;
            on_temperature_changed.emit(temp);
        }
    }
private:
    float temperature_ = 0.0f;
};

int main() {
    TemperatureSensor sensor;

    auto id1 = sensor.on_temperature_changed.connect([](float temp) {
        std::print("Display: {:.1f} C\n", temp);
    });

    auto id2 = sensor.on_temperature_changed.connect([](float temp) {
        if (temp > 40.0f)
            std::print("ALERT: critical temperature!\n");
    });

    sensor.set_temperature(25.0f);
    sensor.set_temperature(42.0f);

    sensor.on_temperature_changed.disconnect(id1);
    std::print("After disconnect ({} observers):\n", sensor.on_temperature_changed.size());
    sensor.set_temperature(10.0f);
}
