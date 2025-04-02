#pragma once

#include <string>
// #include <format>
#include <optional>
#include <set>

class HandleManager {
public:
    std::string freshID(){
        char buffer[15]{};
        snprintf(buffer, std::size(buffer), "%012lld", current_id++);
        return {buffer};
        // return std::format("{:012d}", current_id++);  // C++20
    }

    std::int64_t freshHandler(){
        return current_id++;
    }

    // std::optional<Image> operator[](std::string id){
    //     return {};
    // }

private:
    std::int64_t current_id;
};
