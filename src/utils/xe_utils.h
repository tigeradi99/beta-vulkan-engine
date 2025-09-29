//
// Created by adity on 22-07-2025.
//

#pragma once

#include <functional>

namespace xe {

    // from: https://stackoverflow.com/a/57595105
    template <typename T, typename... Rest>
    void hashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
        seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        (hashCombine(seed, rest), ...);
    };

    struct AABB3 {
        float minX, minY, minZ;
        float maxX, maxY, maxZ;
    };

}  // namespace xe
