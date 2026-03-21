/* ============================================================================
   Section 24.6.3 : RE2 et PCRE2 — Alternatives runtime performantes
   Description : PCRE2 wrapper RAII avec JIT, lookbehind, iteration
   Fichier source : 06.3-re2-pcre2.md
   ============================================================================ */

// Compilation : g++-15 -std=c++23 -O2 -o 06_3_pcre2 06_3_pcre2.cpp -lpcre2-8

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>
#include <optional>
#include <print>

class Regex {
public:
    explicit Regex(std::string_view pattern, uint32_t options = 0) {
        int errornumber;
        PCRE2_SIZE erroroffset;
        code_ = pcre2_compile(
            reinterpret_cast<PCRE2_SPTR>(pattern.data()),
            pattern.size(), options,
            &errornumber, &erroroffset, nullptr);
        if (!code_) {
            PCRE2_UCHAR buf[256];
            pcre2_get_error_message(errornumber, buf, sizeof(buf));
            throw std::runtime_error(
                "Regex compile error at offset " + std::to_string(erroroffset) +
                ": " + reinterpret_cast<char*>(buf));
        }
        pcre2_jit_compile(code_, PCRE2_JIT_COMPLETE);
        match_data_ = pcre2_match_data_create_from_pattern(code_, nullptr);
    }

    ~Regex() {
        if (match_data_) pcre2_match_data_free(match_data_);
        if (code_) pcre2_code_free(code_);
    }

    Regex(const Regex&) = delete;
    Regex& operator=(const Regex&) = delete;

    auto search(std::string_view subject, PCRE2_SIZE start = 0) const
        -> std::optional<std::vector<std::string_view>>
    {
        int rc = pcre2_match(code_,
            reinterpret_cast<PCRE2_SPTR>(subject.data()),
            subject.size(), start, 0, match_data_, nullptr);
        if (rc < 0) return std::nullopt;
        PCRE2_SIZE* ov = pcre2_get_ovector_pointer(match_data_);
        std::vector<std::string_view> captures;
        captures.reserve(static_cast<std::size_t>(rc));
        for (int i = 0; i < rc; ++i) {
            if (ov[2*i] == PCRE2_UNSET) captures.emplace_back();
            else captures.emplace_back(subject.data() + ov[2*i], ov[2*i+1] - ov[2*i]);
        }
        return captures;
    }

private:
    pcre2_code* code_ = nullptr;
    pcre2_match_data* match_data_ = nullptr;
};

int main() {
    // Lookbehind (impossible avec RE2)
    std::println("=== PCRE2 lookbehind ===");
    Regex price_pattern(R"((?<=\$)\d+(?:\.\d{2})?)");
    std::string_view text = "Prix: $42.99 et $100 et €50";
    PCRE2_SIZE offset = 0;
    while (auto captures = price_pattern.search(text, offset)) {
        auto full = (*captures)[0];
        std::println("Prix USD : {}", full);
        offset = static_cast<PCRE2_SIZE>(full.data() - text.data() + full.size());
    }

    // Captures classiques
    std::println("\n=== PCRE2 captures ===");
    Regex date_re(R"((\d{4})-(\d{2})-(\d{2}))");
    if (auto caps = date_re.search("Date: 2026-03-21")) {
        std::println("Date: {}, Année: {}, Mois: {}, Jour: {}",
            (*caps)[0], (*caps)[1], (*caps)[2], (*caps)[3]);
    }
}
