#include <iostream>
#include <memory>
#include <map>
#include <string>
#include <array>
#include <vector>
#include <cassert>
#include <plugin/dynamic_library.hpp>
#include <plugin/plugin_manager.hpp>
#include <core/project_data.hpp>
#include <memory/memory_manager.hpp>
#include <render/rendering_scheduler.hpp>
#include <util/dump.hpp>


int main(){
    using namespace PrismCascade;
    try{
        std::cout << "Hello world" << std::endl;
    }catch(const std::exception& e){
        std::cerr << e.what() << std::endl;
    }
}
