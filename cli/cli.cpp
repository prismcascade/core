#include <iostream>
#include <prismcascade/memory/time.hpp>
#include <prismcascade/plugin/dynamic_library.hpp>

int main() {
    using namespace prismcascade;
    std::cout << "Hello world!" << std::endl;

    std::cout << "----------------" << std::endl;

    auto plugin_file_list = plugin::DynamicLibrary::list_plugins();
    for (auto&& plugin_file : plugin_file_list) std::cout << " - " << plugin_file << std::endl;

    std::cout << "----------------" << std::endl;

    plugin::DynamicLibrary lib(plugin_file_list.at(0));
    bool                   result = reinterpret_cast<bool (*)(int)>(lib["hello_world"])(42);
    std::cout << "result = " << result << std::endl;
}
