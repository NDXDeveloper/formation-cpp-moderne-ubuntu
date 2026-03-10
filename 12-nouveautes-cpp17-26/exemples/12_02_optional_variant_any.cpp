/* ============================================================================
   Section 12.2 : std::optional, std::variant, std::any (C++17)
   Description : Exemples complets des trois types vocabulaires - optional
                 (construction, acces, monadic C++23), variant (visit,
                 overloaded, machine a etats, monostate), any (cast, API)
   Fichier source : 02-optional-variant-any.md
   ============================================================================ */
#include <optional>
#include <variant>
#include <any>
#include <string>
#include <map>
#include <print>
#include <charconv>
#include <vector>

using namespace std::string_literals;

// === std::optional - find_score (lignes 40-52) ===
std::map<std::string, int> scores = {{"Alice", 95}, {"Bob", 87}};

std::optional<int> find_score(const std::string& name) {
    if (auto it = scores.find(name); it != scores.end()) {
        return it->second;
    }
    return std::nullopt;
}

// === std::optional - find_entry (lignes 140-146) ===
std::optional<std::pair<std::string, int>> find_entry(const std::string& key) {
    static std::map<std::string, int> db = {{"timeout", 30}, {"retries", 3}};
    if (auto it = db.find(key); it != db.end()) {
        return *it;
    }
    return std::nullopt;
}

// === Monadic operations helpers (lignes 165-176) ===
std::optional<std::string> get_env(const std::string& name) {
    if (name == "PORT") return "8080";
    return std::nullopt;
}

std::optional<int> parse_int(const std::string& s) {
    int value{};
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (ec == std::errc{}) return value;
    return std::nullopt;
}

// === Overloaded helper (lignes 303-304) ===
template <class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

// === Machine a etats (lignes 353-371) ===
struct Idle {};
struct Connecting { std::string host; int port; };
struct Connected { int socket_fd; };
struct Error { std::string message; };

using ConnectionState = std::variant<Idle, Connecting, Connected, Error>;

int main() {
    // --- optional: basic usage (lignes 57-67) ---
    std::print("=== optional basic ===\n");
    auto result = find_score("Alice");
    if (result) {
        std::print("Score : {}\n", *result);
    }
    int charlie_score = find_score("Charlie").value_or(0);
    std::print("Score de Charlie : {}\n", charlie_score);

    // --- optional: construction (lignes 76-106) ---
    std::print("\n=== optional construction ===\n");
    std::optional<std::string> empty;
    std::optional<std::string> also_empty = std::nullopt;
    std::optional<std::string> name_opt = "Alice";
    std::optional<int> score_opt{95};
    std::optional<std::string> built(std::in_place, 5, 'x');
    std::print("empty={}, also_empty={}, name={}, score={}, built={}\n",
        empty.has_value(), also_empty.has_value(), *name_opt, *score_opt, *built);

    auto mk_name = std::make_optional<std::string>("Alice");
    auto mk_score = std::make_optional(95);
    std::print("make_optional: name={}, score={}\n", *mk_name, *mk_score);

    std::optional<int> value;
    value = 42;
    std::print("value={}\n", *value);
    value = std::nullopt;
    std::print("has_value={}\n", value.has_value());
    value = 100;
    value.reset();
    std::print("after reset: has_value={}\n", value.has_value());

    // --- optional: access (lignes 112-126) ---
    std::print("\n=== optional access ===\n");
    std::optional<std::string> acc_name = "Alice";
    std::string s1 = *acc_name;
    std::size_t len = acc_name->size();
    std::string s2 = acc_name.value();
    std::string s3 = acc_name.value_or("inconnu");
    std::print("s1={}, len={}, s2={}, s3={}\n", s1, len, s2, s3);

    // --- optional: structured binding combo (lignes 149-152) ---
    std::print("\n=== optional + structured binding ===\n");
    if (auto entry = find_entry("timeout"); entry) {
        auto [key, val] = *entry;
        std::print("{} = {}\n", key, val);
    }

    // --- optional: monadic operations C++23 (lignes 178-189) ---
    std::print("\n=== optional monadic (C++23) ===\n");
    std::optional<int> port = get_env("PORT").and_then(parse_int);
    std::print("port={}\n", port.value_or(-1));

    std::optional<std::string> port_str = get_env("PORT")
        .transform([](const std::string& s) { return "Port: " + s; });
    std::print("port_str={}\n", port_str.value_or("none"));

    std::optional<std::string> fallback = get_env("CUSTOM_PORT")
        .or_else([]() { return get_env("PORT"); });
    std::print("fallback={}\n", fallback.value_or("none"));

    // --- variant: basic (lignes 237-253) ---
    std::print("\n=== variant basic ===\n");
    std::variant<int, double, std::string> var_val;
    var_val = 42;
    std::print("index={}\n", var_val.index());
    var_val = 3.14;
    var_val = "hello"s;
    std::print("string={}\n", std::get<std::string>(var_val));

    // --- variant: access (lignes 260-275) ---
    std::print("\n=== variant access ===\n");
    std::variant<int, double, std::string> v = 42;
    int i = std::get<int>(v);
    int j = std::get<0>(v);
    std::print("get<int>={}, get<0>={}\n", i, j);

    if (auto* ptr = std::get_if<int>(&v)) {
        std::print("C'est un int : {}\n", *ptr);
    }
    std::print("index={}\n", v.index());

    // --- variant: visit (lignes 284-314) ---
    std::print("\n=== variant visit ===\n");
    using JsonValue = std::variant<int, double, bool, std::string>;
    JsonValue jval = "hello"s;

    std::visit([](const auto& val) {
        std::print("Valeur : {}\n", val);
    }, jval);

    jval = 3.14;
    std::visit(overloaded{
        [](int vi)                { std::print("Entier : {}\n", vi); },
        [](double d)             { std::print("Flottant : {:.2f}\n", d); },
        [](bool b)               { std::print("Booleen : {}\n", b); },
        [](const std::string& s) { std::print("Chaine : '{}'\n", s); }
    }, jval);

    // --- variant: machine a etats (lignes 350-371) ---
    std::print("\n=== variant state machine ===\n");
    ConnectionState state = Idle{};
    state = Connecting{"api.example.com", 443};

    std::visit(overloaded{
        [](const Idle&)         { std::print("Idle\n"); },
        [](const Connecting& c) { std::print("Connecting to {}:{}\n", c.host, c.port); },
        [](const Connected& c)  { std::print("Connected fd={}\n", c.socket_fd); },
        [](const Error& e)      { std::print("Error: {}\n", e.message); }
    }, state);

    // --- variant: monostate (lignes 381-392) ---
    std::print("\n=== monostate ===\n");
    struct NoDefault {
        NoDefault(int v) : val(v) {}
        int val;
    };
    std::variant<std::monostate, NoDefault, int> mono_v;
    std::print("monostate index={}\n", mono_v.index());

    // --- any: basic (lignes 427-451) ---
    std::print("\n=== any basic ===\n");
    std::any any_val;
    any_val = 42;
    any_val = std::string("hello");
    any_val = 3.14;

    try {
        double d = std::any_cast<double>(any_val);
        std::print("Valeur : {}\n", d);
    } catch (const std::bad_any_cast& e) {
        std::print("Mauvais type !\n");
    }

    if (auto* ptr = std::any_cast<double>(&any_val)) {
        std::print("C'est un double : {}\n", *ptr);
    }

    // --- any: API (lignes 457-473) ---
    std::print("\n=== any API ===\n");
    std::any a = 42;
    std::print("has_value={}\n", a.has_value());
    std::print("type==int: {}\n", a.type() == typeid(int));
    a.reset();
    std::print("after reset: has_value={}\n", a.has_value());

    a.emplace<std::string>(5, 'x');
    std::print("emplace: {}\n", std::any_cast<std::string>(a));

    auto b = std::make_any<std::string>("hello");
    std::print("make_any: {}\n", std::any_cast<std::string>(b));
}
