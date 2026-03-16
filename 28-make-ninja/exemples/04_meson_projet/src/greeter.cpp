#include "greeter.h"

namespace greeter {
std::string greet(const std::string& name) {
    return "Hello from Meson, " + name + "!";
}
}
