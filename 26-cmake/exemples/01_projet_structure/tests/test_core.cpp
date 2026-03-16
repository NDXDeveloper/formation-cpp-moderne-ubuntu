#include <my_project/core.h>
#include <cassert>
#include <print>

int main() {
    auto greeting = my_project::get_greeting("world");
    assert(greeting == "[Hello, WORLD!]");
    std::println("test_core PASSED: {}", greeting);
    return 0;
}
