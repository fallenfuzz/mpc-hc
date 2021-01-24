#pragma once

bool RetrieveBitmapData(unsigned w, unsigned h, unsigned bpp, BYTE* dst, BYTE* src, int srcpitch);

// checks the multiplicity of the angle of 90 degrees and brings it to the values of 0, 90, 270
bool AngleStep90(int& angle);
