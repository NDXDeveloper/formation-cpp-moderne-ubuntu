/* ============================================================================
   Section 11.1.3 : Capture de this
   Description : Exemples complets de capture de this — [this] vs [*this],
                 dangling, snapshot, mutable, init capture de membres,
                 héritage, polymorphisme, shared_from_this
   Fichier source : 01.3-capture-this.md
   ============================================================================ */
#include <print>
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <format>
#include <span>
#include <algorithm>

// === [this] capture (ligne 32-44) ===
class Sensor {
    std::string name_ = "temp_01";
    double value_ = 23.5;
public:
    auto create_reporter() {
        return [this]() {
            std::print("{}: {}\n", name_, value_);
        };
    }

    // Pour [*this] (ligne 175-185)
    auto create_reporter_copy() const {
        return [*this]() {
            std::print("{}: {}\n", name_, value_);
        };
    }

    void update(double v) { value_ = v; }
};

// === Counter (ligne 71-86) ===
class Counter {
    int count_ = 0;
public:
    auto make_incrementer() {
        return [this]() {
            ++count_;
            return count_;
        };
    }

    // [*this] mutable (ligne 243-258)
    auto make_independent_counter() {
        return [*this]() mutable {
            return ++count_;
        };
    }

    int get_count() const { return count_; }
};

// === Config (ligne 221-236) ===
class Config {
    int timeout_ = 30;
public:
    auto snapshot() const {
        return [*this]() { return timeout_; };
    }
    void set_timeout(int t) { timeout_ = t; }
};

// === Processor — init capture de membres (ligne 344-357) ===
class Processor {
    int threshold_ = 100;
    std::vector<double> data_;
public:
    auto make_filter() {
        return [threshold = threshold_](double v) {
            return v > threshold;
        };
    }
};

// === Connection — init capture sélective (ligne 363-374) ===
class Connection {
    std::string host_ = "localhost";
    int port_ = 8080;
    std::vector<char> buffer_;
public:
    auto make_reconnector() {
        return [host = host_, port = port_]() {
            std::print("Reconnecting to {}:{}...\n", host, port);
        };
    }
};

// === Hiérarchie de classes (ligne 386-400) ===
class Base {
protected:
    int id_ = 0;
};

class Derived : public Base {
    std::string label_ = "default";
public:
    auto create_identifier() {
        return [this]() {
            return std::format("{}:{}", id_, label_);
        };
    }
};

// === Polymorphisme (ligne 406-428) ===
class Shape {
public:
    virtual std::string name() const { return "Shape"; }
    auto make_printer() {
        return [this]() {
            std::print("I am a {}\n", name());
        };
    }
    virtual ~Shape() = default;
};

class Circle : public Shape {
public:
    std::string name() const override { return "Circle"; }
};

// === shared_from_this (ligne 446-466) ===
class Session : public std::enable_shared_from_this<Session> {
    std::string id_ = "sess_001";
public:
    auto create_handler() {
        auto self = shared_from_this();
        return [self]() {
            std::print("Handling session: {}\n", self->id_);
        };
    }
};

int main() {
    // --- [this] capture (ligne 32-44) ---
    {
        Sensor s;
        auto reporter = s.create_reporter();
        reporter();  // temp_01: 23.5
    }

    // --- Counter [this] (ligne 71-86) ---
    {
        Counter c;
        auto inc = c.make_incrementer();
        std::print("{}\n", inc());  // 1
        std::print("{}\n", inc());  // 2
    }

    // --- [*this] capture (ligne 175-214) ---
    {
        std::function<void()> callback;
        {
            Sensor s;
            callback = s.create_reporter_copy();
        }  // s est détruit
        callback();  // temp_01: 23.5 — copie sûre
    }

    // --- Config snapshot [*this] (ligne 221-236) ---
    {
        Config cfg;
        auto snap = cfg.snapshot();
        cfg.set_timeout(60);
        std::print("{}\n", snap());  // 30 — snapshot avant modification
    }

    // --- Counter [*this] mutable (ligne 243-258) ---
    {
        Counter c;
        auto counter = c.make_independent_counter();
        std::print("{}\n", counter());  // 1
        std::print("{}\n", counter());  // 2
        std::print("original count: {}\n", c.get_count());  // 0
    }

    // --- Init capture de membres (ligne 344-374) ---
    {
        Processor p;
        auto filter = p.make_filter();
        std::print("filter(50) = {}, filter(150) = {}\n", filter(50), filter(150));
        // false, true
    }
    {
        Connection conn;
        auto reconnector = conn.make_reconnector();
        reconnector();  // Reconnecting to localhost:8080...
    }

    // --- Hiérarchie — Derived (ligne 386-400) ---
    {
        Derived d;
        auto id = d.create_identifier();
        std::print("{}\n", id());  // 0:default
    }

    // --- Polymorphisme virtuel (ligne 406-428) ---
    {
        Circle c;
        auto printer = c.make_printer();
        printer();  // I am a Circle
    }

    // --- shared_from_this (ligne 446-466) ---
    {
        std::function<void()> handler;
        {
            auto session = std::make_shared<Session>();
            handler = session->create_handler();
        }
        handler();  // Handling session: sess_001
    }

    return 0;
}
