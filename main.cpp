#include "engine.h"

#include <iostream>
#include <dxgi1_6.h>
#include <d3d12.h>
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")

// int main() {
//     try {
//         Engine eng{};
//         eng.render();
//     } catch(const std::exception& e) {
//         std::cerr << "Fatal error: " << e.what() << std::endl;
//     }
//     return 0;
// }
