#pragma once
#include <string>
#include <algorithm>

namespace utils {

/// Convertit une chaîne en majuscules
inline std::string to_upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

} // namespace utils
