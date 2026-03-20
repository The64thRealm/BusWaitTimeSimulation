//
// Created by The 64th Realm on 03/16/26.
//

#pragma once

#include <random>

static std::mt19937& globalRng()
{
    static std::mt19937 rng = []
    {
        std::random_device rd;
        std::seed_seq seed{rd(), rd(), rd(), rd()};
        return std::mt19937(seed);
    }();
    return rng;
}
