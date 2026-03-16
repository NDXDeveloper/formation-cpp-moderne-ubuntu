#include <my_project/version.h>
#include <my_project/config.h>
#include <my_project/build_info.h>
#include <print>

int main() {
    std::println("=== Version ===");
    std::println("Version : {}", my_project_VERSION);
    std::println("Major   : {}", my_project_VERSION_MAJOR);
    std::println("Minor   : {}", my_project_VERSION_MINOR);
    std::println("Patch   : {}", my_project_VERSION_PATCH);

    std::println("\n=== Configuration ===");
    std::println("System  : {}", MY_PROJECT_SYSTEM_NAME);
    std::println("Compiler: {} {}", MY_PROJECT_COMPILER_ID, MY_PROJECT_COMPILER_VER);
    std::println("Build   : {}", MY_PROJECT_BUILD_TYPE);
    std::println("ZLIB    : {}", MY_PROJECT_HAS_ZLIB ? "oui" : "non");
    std::println("SSL     : {}", MY_PROJECT_HAS_SSL ? "oui" : "non");

    std::println("\n=== Build Info ===");
    std::println("Commit  : {}", MY_PROJECT_GIT_HASH);
    std::println("Date    : {}", MY_PROJECT_BUILD_DATE);

    return 0;
}
