//
// Created by The 64th Realm on 03/16/26.
//

#include <random>
#include <algorithm>
#include <iostream>
#include <boost/math/distributions/normal.hpp>
#include "rngInitializer.h"
#include "waitStats.h"

#define SET_STD_DEV
// #define WAIT_FOR_BUS_TIME

// the earliest the bus is allowed to leave
constexpr static float MAXIMUM_BUS_EARLY_LEAVE_TIME = 0.f;

constexpr static float BUS_INTERVAL = 12.f;

static constexpr size_t NUM_BUSES = 100000;
constexpr static float BUS_LATE_TIME = 5.f;
constexpr static float BUS_LATE_PERCENTAGE = 0.2f;

#ifdef SET_STD_DEV
static float busArrivalStdDev = 2.f;
#else
// metro buses are late like 20% of the time
static float busArrivalStdDev = []
{
    boost::math::normal_distribution<float> busNormal(0.f, 1.f);
    float z = boost::math::quantile(busNormal, 1.f - BUS_LATE_PERCENTAGE);
    return BUS_LATE_TIME / z;
}();
#endif

static std::normal_distribution<float> normalDist(0.f, busArrivalStdDev);

constexpr float normalizeTime(float input)
{
#ifdef WAIT_FOR_BUS_TIME
    return std::max(-MAXIMUM_BUS_EARLY_LEAVE_TIME, input);
#else
    return input;
#endif
}

size_t generateBusTimes(std::vector<float>& busTimesVector)
{
    busTimesVector.reserve(NUM_BUSES);
    size_t lateBusses = 0;
    for (size_t i = 1; i <= NUM_BUSES; ++i)
    {
        float offset = normalizeTime(normalDist(globalRng()));
        if (offset > BUS_LATE_TIME)
            ++lateBusses;
        busTimesVector.push_back(static_cast<float>(i) * BUS_INTERVAL + offset);
    }

    std::ranges::sort(busTimesVector);
    return lateBusses;
}

int main()
{
    std::vector<float> busTimesVector;
    size_t numLateBuses = generateBusTimes(busTimesVector);
    getStopStats(busTimesVector, numLateBuses, BUS_INTERVAL);
    printExpectedDistributionStats(BUS_INTERVAL, busArrivalStdDev);
}