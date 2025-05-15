#if defined(_WIN32) || defined(_WIN64)
#define _IS_WINDOWS_BUILD
#endif

#ifdef _IS_WINDOWS_BUILD
#define EXPORT __declspec(dllexport)
#define API_CALL __cdecl
#else
#define EXPORT __attribute__((visibility("default")))
#define API_CALL
#endif

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

extern "C" {

EXPORT bool API_CALL hello_world(int t) {
    std::cout << "Hello world: [" << t << "]" << std::endl;
    return true;
}
}
