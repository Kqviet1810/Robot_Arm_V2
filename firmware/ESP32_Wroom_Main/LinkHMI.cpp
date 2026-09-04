#include "LinkHMI.h"

LinkHMI *LinkHMI::self_ = nullptr;
constexpr uint8_t LinkHMI::kBroadcastAddr[6];
