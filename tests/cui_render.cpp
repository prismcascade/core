#include <iostream>
#include <memory>
#include <functional>
#include <plugin/dynamic_library.hpp>

int main(){
    try{
        DynamicLibrary lib("test_library.dll");
        void* ptr = lib["Handle"];
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
    } catch(...) {
        std::cout << "Error (Loading File)" << std::endl;
    }
}
