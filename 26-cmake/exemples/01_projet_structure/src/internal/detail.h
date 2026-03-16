#pragma once
#include <string>

namespace my_project::detail {

/// Formate une chaîne entre crochets (header privé, non exposé)
inline std::string format_internal(const std::string& s) {
    return "[" + s + "]";
}

} // namespace my_project::detail
