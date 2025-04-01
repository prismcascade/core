#pragma once

#include <string>
#include <format>
#include <optional>
#include <set>

class PngSequenceManager {
public:
    std::string freshID(){
        return std::format("{:012d}", current_id++);
    }

    // std::optional<Image> operator[](std::string id){
    //     return {};
    // }

private:
    long current_id = 0;
};
