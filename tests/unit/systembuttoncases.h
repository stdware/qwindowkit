// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <limits>

namespace QwkTest {
    inline constexpr struct {
        const char *name;
        int value;
        bool valid;
    } systemButtonCases[] = {
        {"int-min", std::numeric_limits<int>::min(), false},
        {"negative", -1, false},
        {"unknown", 0, false},
        {"icon", 1, true},
        {"help", 2, true},
        {"minimize", 3, true},
        {"maximize", 4, true},
        {"close", 5, true},
        {"past-close", 6, false},
        {"unnamed-representable", 7, false},
        {"int-max", std::numeric_limits<int>::max(), false},
    };
}
