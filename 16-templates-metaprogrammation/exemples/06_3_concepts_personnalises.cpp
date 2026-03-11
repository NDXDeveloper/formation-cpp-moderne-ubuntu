/* ============================================================================
   Section 16.6.3 : Création de concepts personnalisés
   Description : Numeric/SignedNumeric/SortableRange, Serializable, StlContainer,
                 InsertableInto, hiérarchie Named/Describable/JsonExportable,
                 LogPolicy/Service, buffer_traits/Bufferable, FilterableBy
   Fichier source : 06.3-concepts-personnalises.md
   ============================================================================ */
#include <print>
#include <concepts>
#include <string>
#include <string_view>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <ranges>
#include <format>
#include <algorithm>

using namespace std::string_literals;

// === Numeric / SignedNumeric / SortableRange ===
template <typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

template <typename T>
concept SignedNumeric = (Numeric<T> && std::signed_integral<T>)
                                   || std::floating_point<T>;

template <typename R>
concept SortableRange = std::ranges::random_access_range<R>
                     && std::totally_ordered<std::ranges::range_value_t<R>>;

static_assert(SortableRange<std::vector<int>>);
static_assert(!SortableRange<std::list<int>>);

// === Serializable ===
template <typename T>
concept Serializable = requires(const T& obj) {
    { obj.serialize() } -> std::convertible_to<std::string>;
};

struct Config {
    std::string name;
    int version;
    std::string serialize() const {
        return std::format("{}:v{}", name, version);
    }
};
struct RawData { void* ptr; std::size_t len; };

static_assert(Serializable<Config>);
static_assert(!Serializable<RawData>);
static_assert(!Serializable<int>);

// === StlContainer ===
template <typename T>
concept StlContainer = requires(T c) {
    typename T::value_type;
    typename T::iterator;
    typename T::size_type;
    { c.begin() }  -> std::same_as<typename T::iterator>;
    { c.end() }    -> std::same_as<typename T::iterator>;
    { c.size() }   -> std::convertible_to<typename T::size_type>;
    { c.empty() }  -> std::convertible_to<bool>;
};

static_assert(StlContainer<std::vector<int>>);
static_assert(StlContainer<std::list<double>>);
static_assert(!StlContainer<int>);

// === InsertableInto ===
template <typename T, typename Container>
concept InsertableInto = requires(Container& c, T val) {
    c.push_back(val);
} || requires(Container& c, T val) {
    c.insert(val);
};

template <typename T, typename Container>
    requires InsertableInto<T, Container>
void ajouter(Container& c, const T& val) {
    if constexpr (requires { c.push_back(val); }) {
        c.push_back(val);
    } else {
        c.insert(val);
    }
}

// === Named / Describable / JsonExportable ===
template <typename T>
concept Named = requires(const T& obj) {
    { obj.name() } -> std::convertible_to<std::string_view>;
};

template <typename T>
concept Describable = Named<T> && requires(const T& obj) {
    { obj.description() } -> std::convertible_to<std::string>;
};

template <typename T>
concept JsonExportable = Describable<T> && requires(const T& obj) {
    { obj.to_json() } -> std::convertible_to<std::string>;
};

template <Named T>
std::string afficher_named(const T& obj) {
    return std::format("Nom : {}", obj.name());
}

template <Describable T>
std::string afficher_named(const T& obj) {
    return std::format("{}: {}", obj.name(), obj.description());
}

template <JsonExportable T>
std::string afficher_named(const T& obj) {
    return obj.to_json();
}

struct SimpleNamed {
    std::string_view name() const { return "Simple"; }
};
struct DescNamed {
    std::string_view name() const { return "Desc"; }
    std::string description() const { return "descriptible"; }
};
struct JsonNamed {
    std::string_view name() const { return "Json"; }
    std::string description() const { return "exportable JSON"; }
    std::string to_json() const { return R"({"name":"Json"})"; }
};

// === LogPolicy ===
template <typename P>
concept LogPolicy = requires(P policy, std::string_view message, int level) {
    { policy.write(message, level) };
    { P::min_level } -> std::convertible_to<int>;
};

struct ConsoleLog {
    static constexpr int min_level = 0;
    void write(std::string_view msg, int level) const {
        if (level >= min_level) std::print("[{}] {}\n", level, msg);
    }
};
struct NullLog {
    static constexpr int min_level = 999;
    void write(std::string_view, int) const {}
};

static_assert(LogPolicy<ConsoleLog>);
static_assert(LogPolicy<NullLog>);

template <LogPolicy Logger>
class Service {
public:
    void execute() {
        logger_.write("Service demarre", 1);
        logger_.write("Service termine", 1);
    }
private:
    Logger logger_;
};

// === buffer_traits + Bufferable ===
template <typename T> struct buffer_traits;

template <> struct buffer_traits<float> {
    static constexpr std::size_t default_size = 1024;
    static constexpr bool gpu_compatible = true;
};
template <> struct buffer_traits<double> {
    static constexpr std::size_t default_size = 512;
    static constexpr bool gpu_compatible = true;
};

template <typename T>
concept Bufferable = requires {
    { buffer_traits<T>::default_size } -> std::convertible_to<std::size_t>;
    { buffer_traits<T>::gpu_compatible } -> std::convertible_to<bool>;
};

static_assert(Bufferable<float>);
static_assert(Bufferable<double>);

// === FilterableBy ===
template <typename Container, typename Pred>
concept FilterableBy = std::ranges::input_range<Container>
    && std::predicate<Pred, std::ranges::range_value_t<Container>>;

template <typename Container, typename Pred>
    requires FilterableBy<Container, Pred>
auto filtrer(const Container& c, Pred pred) {
    using T = std::ranges::range_value_t<Container>;
    std::vector<T> result;
    for (const auto& elem : c)
        if (pred(elem)) result.push_back(elem);
    return result;
}

int main() {
    // Serializable
    {
        Config cfg{"app", 3};
        std::print("{}\n", cfg.serialize());  // app:v3
    }

    // InsertableInto
    {
        std::vector<int> v;
        ajouter(v, 42);
        std::print("vector: {}\n", v[0]);  // 42

        std::set<int> s;
        ajouter(s, 42);
        std::print("set size: {}\n", s.size());  // 1
    }

    // Subsomption Named/Describable/JsonExportable
    {
        std::print("{}\n", afficher_named(SimpleNamed{}));  // Nom : Simple
        std::print("{}\n", afficher_named(DescNamed{}));    // Desc: descriptible
        std::print("{}\n", afficher_named(JsonNamed{}));    // {"name":"Json"}
    }

    // Service avec LogPolicy
    {
        Service<ConsoleLog> s1;
        s1.execute();

        Service<NullLog> s2;
        s2.execute();  // silencieux
    }

    // filtrer
    {
        std::vector<int> nums{1, 2, 3, 4, 5, 6, 7, 8};
        auto pairs = filtrer(nums, [](int n) { return n % 2 == 0; });
        for (auto n : pairs) std::print("{} ", n);
        std::print("\n");  // 2 4 6 8
    }
}
