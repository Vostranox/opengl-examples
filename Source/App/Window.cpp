#include "App/Window.hpp"

#if defined(_WIN32)
extern "C" {
__declspec(dllexport) unsigned int NvOptimusEnablement         = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif
