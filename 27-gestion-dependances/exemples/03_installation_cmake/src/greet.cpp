#include <mylib/greet.h>

namespace mylib {
std::string greet(const std::string& name) {
    return "Hello, " + name + "!";
}
}
