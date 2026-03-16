#include <my_project/core.h>
#include <my_project/network.h>
#include <print>

int main() {
    std::println("{}", my_project::get_greeting("world"));
    std::println("Host: {}", my_project::get_host_info());
    return 0;
}
