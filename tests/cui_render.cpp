#include <iostream>
#include <memory>
#include <map>
#include <string>
#include <array>
#include <vector>
#include <cassert>

int main(){
    // using namespace PrismCascade;
    try{
        std::cout << "Hello world" << std::endl;
    }catch(const std::exception& e){
        std::cerr << e.what() << std::endl;
    }
}
