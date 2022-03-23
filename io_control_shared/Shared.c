#include "Shared.h"

void EnsureSafeConfig(PCMDD_MOUSE_CONFIG config) {
    if (!IsSafeDPI(config->OriginalDPI)) {
        config->OriginalDPI = CMDD_DEFAULT_CONFIG_ORIGINAL_DPI;
    }

    if (!IsSafeDPI(config->TargetDPIX)) {
        config->TargetDPIX = CMDD_DEFAULT_CONFIG_TARGET_DPI_X;
    }

    if (!IsSafeDPI(config->TargetDPIY)) {
        config->TargetDPIY = CMDD_DEFAULT_CONFIG_TARGET_DPI_Y;
    }
}