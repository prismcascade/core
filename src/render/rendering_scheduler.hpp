#pragma once

#include <string>
// #include <format>
#include <optional>
#include <set>

class HandleManager {
public:
    std::string freshID(){
        char buffer[15]{};
        snprintf(buffer, std::size(buffer), "%012ld", current_id++);
        return {buffer};
        // return std::format("{:012d}", current_id++);  // C++20
    }

    int freshHandler(){
        return current_id++;
    }

    // std::optional<Image> operator[](std::string id){
    //     return {};
    // }

private:
    long current_id;
};
