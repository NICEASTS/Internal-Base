#pragma once
#include "../../../sdk/utils/Globals.h"
#include "../../../sdk/utils/Vector.h"
#include <cstdint>

namespace RCS {
    void Run();
    Vector GetAimCompensation(uintptr_t pawnAddr, Globals::AimWeaponGroup group);
}