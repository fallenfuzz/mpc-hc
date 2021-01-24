#include "stdafx.h"
#include <memory>

#include "Utils.h"

bool RetrieveBitmapData(unsigned w, unsigned h, unsigned bpp, BYTE* dst, BYTE* src, int srcpitch) {
    unsigned linesize = w * bpp / 8;
    if ((int)linesize > srcpitch) {
        return false;
    }

    src += srcpitch * (h - 1);

    for (unsigned y = 0; y < h; ++y) {
        memcpy(dst, src, linesize);
        src -= srcpitch;
        dst += linesize;
    }

    return true;
}

bool AngleStep90(int& angle) {
    if (angle % 90 == 0) {
        angle %= 360;
        if (angle < 0) {
            angle += 360;
        }
        return true;
    }
    return false;
}
