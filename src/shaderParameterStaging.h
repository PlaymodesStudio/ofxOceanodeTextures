#pragma once
#include <algorithm>
#include <vector>

// Stage one parameter block in the CPU mirror of a texture buffer object. The
// mirror is uploaded once per frame with glBufferData, which lets the driver
// orphan the old storage. glBufferSubData on a buffer the GPU may still be
// reading made Apple's GL-on-Metal flush the command stream for every
// parameter of every indexer/oscillator each frame.
inline bool stageShaderParameters(std::vector<float>& mirror, int position, int size, const std::vector<float>& values){
    if(size <= 0 || values.empty() || position < 0) return false;
    if(size_t(position) + size_t(size) > mirror.size()) return false;
    if(values.size() == size_t(size)) std::copy(values.begin(), values.end(), mirror.begin() + position);
    else std::fill(mirror.begin() + position, mirror.begin() + position + size, values[0]);
    return true;
}
