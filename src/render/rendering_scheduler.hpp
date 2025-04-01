#pragma once

#include <string>
#include <format>
#include <optional>
#include <set>

class HandleManager {
public:
    static std::string freshID(){
        return std::format("{:012d}", current_id++);
    }

    static int freshHandler(){
        return current_id++;
    }

    // std::optional<Image> operator[](std::string id){
    //     return {};
    // }

private:
    static long current_id;
};
