//
// Created by The 64th Realm on 03/16/26.
//

#ifndef BUSWAITTIMESIMULATION_CALCULATEBUSSTATS_H
#define BUSWAITTIMESIMULATION_CALCULATEBUSSTATS_H
#include <cmath>
#include <format>
#include <iostream>
#include <numeric>
#include <vector>
#include <boost/math/distributions/normal.hpp>

// #define DEBUG

struct StopStats
{
    static constexpr std::string_view STATS_FORMAT = "{:<8.4f}\t{:<8.4f}\t{:<8.4f}\n";
    static constexpr size_t STATS_FORMAT_LENGTH = 27;

    float avgWait;
    float stdDevWait;
    float percentageOfWaitsLongerThanWaitInterval;

    void appendStatsStringTo(std::string& append)
    {
        std::format_to(std::back_inserter(append), STATS_FORMAT, avgWait, stdDevWait, percentageOfWaitsLongerThanWaitInterval);
    }
};

inline StopStats getStopStats(const std::vector<float>& busTimesVector, const size_t& lateBusCount, const float busInterval, bool print = false)
{
    const float totalTimeInterval = busTimesVector.back() - busTimesVector.front();

    std::vector<float> differences;
    differences.reserve(busTimesVector.size() - 1);

    std::adjacent_difference(busTimesVector.begin(),
        busTimesVector.end(), std::back_inserter(differences));

    float longestWait = 0.f;

    const float avgWait = std::accumulate(differences.begin() + 1, differences.end(), 0.f, [&longestWait](const float acc, const float next)
    {
        longestWait = std::max(longestWait, next);
        return acc + next * next / 2.f;
    }) / totalTimeInterval;

    // percentage of time waited that was longer than the bus interval
    const float badWaitPercentage = std::accumulate(differences.begin() + 1, differences.end(), 0.f, [&busInterval](const float acc, const float next)
    {
        return acc + std::max(0.f, next - busInterval);
    }) / totalTimeInterval;

    // integral of squares of wait times to calculate the variance smoothly
    // integral from 0 to diff of (y - avgWait)^2 dy
    // is equal to ((diff - avgWait)^3 + avgWait^3) / 3
    float cubedWait = avgWait * avgWait * avgWait;
    const float stdDevWait = sqrtf(std::accumulate(differences.begin() + 1, differences.end(), 0.f, [avgWait, cubedWait](const float acc, const float next)
    {
        float diff = next - avgWait;
        return acc + (diff * diff * diff + cubedWait) / 3.f;
    }) / totalTimeInterval);

    static boost::math::normal_distribution<float> actualBusNormal(avgWait, stdDevWait);

    if (print)
    {
        std::cout << "Avg wait: " << avgWait << std::endl;
        std::cout << "StdDev wait: " << stdDevWait << std::endl;
        std::cout << "Avg - Ideal: " << avgWait - busInterval / 2.f << std::endl;
        std::cout << "Percentage of waits longer than the bus interval: " << badWaitPercentage << std::endl;
    // std::cout << "Longest wait: " << longestWait << std::endl;
    }

#ifdef DEBUG
    const float avgIntervalSize = std::accumulate(differences.begin() + 1, differences.end(), 0.f, [](const float acc, const float next)
    {
        return acc + next;
    }) / static_cast<float>(differences.size() - 1);
    const float stdDevInterval = sqrtf(std::accumulate(differences.begin() + 1, differences.end(), 0.f, [busInterval](const float acc, const float next)
    {
        return acc + (busInterval - next) * (busInterval - next);
    }) / static_cast<float>(differences.size() - 2));
    float longerWaitThanIntervalFromDistribution = 1.f - boost::math::cdf(actualBusNormal, busInterval);

    std::cout << "Interval size avg: " << avgIntervalSize << std::endl;
    std::cout << "Interval size stdDeviation: " << stdDevInterval << std::endl;
    std::cout << "Percentage of late buses: " << static_cast<float>(lateBusCount) / static_cast<float>(busTimesVector.size()) << std::endl;
    std::cout << "Percentile of waits longer than the bus interval (calculated, assuming normal distribution): " << longerWaitThanIntervalFromDistribution << std::endl;
#undef DEBUG
#endif
    // for (float difference : differences)
    // {
    //     std::cout << difference << "\n";
    // }

    return {avgWait, stdDevWait, badWaitPercentage};
}

inline void printExpectedDistributionStats(float busInterval, float busStdDev)
{
    std::cout << "This next value is only accurate if the busStdDist is much less than the busInterval, so that buses don't switch order." << std::endl;

    // the below expression is created from:
    // when you integrate from -inf to inf over the bus wait time normal distribution (which will have mean of busInterval and stdDeviation of sqrt(2) * busStdDist because of sum of variances)
    // multiplied with the evaluation expression for the average wait time for an interval of length x = x^2 / 2L where L is the length of the entire simulation
    // to get a weighted sum of the wait times. We'd be also multiplying by the number of intervals because we are adding up that many intervals

    // let n be the number of intervals, L be the length of the entire simulation interval, s be the stdDev of the bus intervals, I be the mean busInterval
    // note that s = sqrt(2) * busStdDev as mentioned previously because the distribution of lengths of bus intervals is the subtraction of the distribution of arrival times
    // from the previous bus to the current bus, and therefore it yields another normal distribution of which we can calculate the mean and stdDev easily

    // this yields the expression: integral from -inf to inf of ( n * (x^2) / 2L    *    e^(-(x - I) ^ 2 / (2 * s ^ 2)) / (s * sqrt(2pi)) dx
    //                                                             ^ avg wait func          ^ normal distribution func

    // not that L / n is the same as the mean interval as the number of samples approaches infinity so we can replace it with I
    // after taking the constants out of the integral it becomes
    // 1 / (2 * I * s * sqrt(2pi)) * integral from -inf to inf of (x*2 * e^(-(x - I) ^ 2 / (2 * s ^ 2)))

    // solving for the indefinite integral becomes https://www.wolframalpha.com/input?i=1%2F%282+*+sigma+*+sqrt%282+*+pi%29%29+*+integral+of+x%5E2+*+e%5E%28%28-1%2F2%29+*+%28%28x-mu%29%2Fsigma%29%5E2%29+dx
    // 1 / (2 * s * I * sqrt(2pi)) * (sqrt(pi / 2) * s * (I^2 + s^2) * erf((x - I) / (sqrt(2) * s)) - s^2 * (I + x) * e^(-(x - I) ^ 2 / (2 * s ^ 2))) from -inf to inf

    // when looking at the part that we have to evaluate from -inf to inf, we see that the 2nd term cancels because as x approaches -inf or inf, the exponent becomes -inf because -(x - I)^2 because very small and therefore e^-(x - I)^2 becomes 0, while the x part becomes inf
    // so it's indeterminate, but e^-x becomes 0 at a faster rate than x becomes inf so this 2nd term just becomes 0 (you can easily verify this with l'hopital)

    // so we only care about the first term, which becomes erf(inf) or erf(-inf). erf(inf) = 1 and erf(-1) = -1 so the expression just becomes:
    // sqrt(pi / 2) * s * (I^2 + s^2) * (1 - -1) / (2 * s * I * sqrt(2pi))
    //                                     ^ this is what the from inf to -inf part turns into when combined with erf

    // and after canceling we can rewrite as
    // (I^2 + s^2) / (2 * I)

    // now rewrite in terms of busStdDev using the sum of variances
    // (I^2 + 2 * busStdDev ^ 2) / (2 * I)
    // I / 2 + busStdDev ^ 2 / I

    // I is just the busInterval, so that's how we got the equation below.
    // unfortunately using this distribution allows for negative bus intervals, which means that if we allow buses to swap places, it won't work because the negative interval doesn't magically become positive
    // well more like it does (since the evaluation function x^2 is an even function), but the corresponding interval that was extended on the other side of the distribution doesn't become shorter
    // so this analysis only works if the buses are swapped rarely

    std::cout << "Expected Average Bus Wait: " << busInterval / 2 + busStdDev * busStdDev / busInterval << std::endl;
}

#endif //BUSWAITTIMESIMULATION_CALCULATEBUSSTATS_H