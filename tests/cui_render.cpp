#include <iostream>
#include <memory>
#include <functional>
#include <plugin/dynamic_library.hpp>

int main(){
    auto lib = std::make_shared<DynamicLibrary>();
    lib->load("test_library.dll");
    if(*lib){
        void* ptr = lib->resolveSymbol("Handle");
        if(ptr){
            std::function<bool(std::string*, int*)> Handle(reinterpret_cast<bool(*)(std::string*, int*)>(ptr));
            std::string buf;
            int t = 1;
            bool success = Handle(&buf, &t);
            std::cout << "(" << success << ") " << std::flush;
            std::cout << buf << std::flush;
            std::cout << ", " << t << std::endl;
            for(char c : buf){
                std::cout << int(c) << ", " << std::flush;
            }
        } else {
            std::cout << "Error (Resolving Handle)" << std::endl;
        }
    } else {
        std::cout << "Error (Loading File)" << std::endl;
    }
}
