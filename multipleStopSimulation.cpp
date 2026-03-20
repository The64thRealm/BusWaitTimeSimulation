//
// Created by The 64th Realm on 03/16/26.
//

// even though 20% of metro buses are late, I'm not sure how to accurately calculate the late percentage using this model
// I may just tweak the numbers until I'm nearly correct

#include <iostream>
#include <boost/math/distributions/normal.hpp>

#include "rngInitializer.h"
#include "waitStats.h"
#include "windowsClipboardHelper.h"

// populates array with
namespace busScheduleCreation {
    constexpr size_t NUM_STOPS = 1000;

    static std::vector<float> busSchedule;

    namespace busScheduleInterval
    {
        constexpr float AVERAGE_TIME = 4.f;
        constexpr float MINIMUM_TIME = 1.f;
        constexpr float STD_DEV_TIME = 1.f;

        float generateBusScheduleInterval()
        {
            static std::normal_distribution<float> normal(AVERAGE_TIME, STD_DEV_TIME);
            return std::max(normal(globalRng()), MINIMUM_TIME);
        }
    }

    void generateBusScheduleIntervals(std::vector<float>& vectorToPopulate)
    {
        vectorToPopulate.reserve(NUM_STOPS);

        float currentTotalTime = 0.f;

        std::generate_n(std::back_inserter(vectorToPopulate), NUM_STOPS, [&currentTotalTime]()
        {
            return currentTotalTime += busScheduleInterval::generateBusScheduleInterval();
        });
    }
}

namespace bus
{
    // it doesn't really particularly matter when the bus arrives at (at least for now) because we don't calculate avg late time for the passengers inside.
    // therefore if 2 buses pass each other (which doesn't really happen irl bc traffic patterns are typically continuous) we just swap the arrival times
    // of the corresponding buses with a sort
    constexpr size_t NUM_BUSES = 100000;
    constexpr float BUS_INTERVAL = 12.f;

    // simulates setting the expected time to be higher than the avg time so that fewer buses will be late
    constexpr float BUS_EXPECTED_TIME_TO_AVG_TIME_RATIO = 0.9f;
    constexpr float BUS_ROUTE_PERCENTAGE_STD_DEV = 1.f;

    float generateBusArrivalTime(float prevTime, float scheduledTripInterval, float scheduledArrivalTime)
    {
        static std::normal_distribution<float> distribution(0.f, 1.f);

        float expectedTripInterval = scheduledTripInterval * BUS_EXPECTED_TIME_TO_AVG_TIME_RATIO;
        float offset = distribution(globalRng()) * expectedTripInterval * BUS_ROUTE_PERCENTAGE_STD_DEV;

        return std::max(expectedTripInterval + offset + prevTime, scheduledArrivalTime);
    }

    // me trying to make stuff cache-friendly, but I think I'll be forced to do at least one cache-unfriendly operation
    // I lied I can just keep the vector so that each outer element is the times at which buses arrive to a particular stop
    // in this model the concept of an individual "bus" doesn't really exist or matter

    // well I guess we're going to be memory-bound since I'm creating the entire vector ahead of time, but I can fix
    // that if I want to do a larger simulation with more buses I guess
    // although increasing the number of buses doesn't really do much probably,
    // so I can just run multiple rounds and get approx the same results I think

    // each element contains a vector of the times at which buses arrive at the station

    void generateBusArrivalTimes(std::vector<std::vector<float>>& busArrivalTimes, std::vector<size_t>& numLateBuses, const std::vector<float>& busScheduleArrivalTimes)
    {
        // precalculate these and store once
        std::vector<float> busScheduleDifferences;
        busScheduleDifferences.reserve(busScheduleArrivalTimes.size());
        std::adjacent_difference(busScheduleArrivalTimes.begin(), busScheduleArrivalTimes.end(), std::back_inserter(busScheduleDifferences));

        busArrivalTimes = std::vector<std::vector<float>>(busScheduleCreation::NUM_STOPS);

        // initialize leave times for the first stop
        numLateBuses.push_back(0); // buses all leave on time on the first stop
        for (size_t busNumber = 0; busNumber < NUM_BUSES; ++busNumber)
        {
            busArrivalTimes[0].push_back(static_cast<float>(busNumber) * BUS_INTERVAL);
        }

        // generate arrival times
        for (size_t stopNumber = 1; stopNumber < busScheduleCreation::NUM_STOPS; ++stopNumber)
        {
            size_t numberOfLateBuses = 0;
            size_t index = 0;
            std::ranges::transform(busArrivalTimes[stopNumber - 1], std::back_inserter(busArrivalTimes[stopNumber]), [&busScheduleArrivalTimes, &stopNumber, &busScheduleDifferences, &index, &numberOfLateBuses](float prevStopTime)
            {
                float scheduledArrivalTime = busScheduleArrivalTimes[stopNumber - 1] + BUS_INTERVAL * static_cast<float>(index++);
                float generatedArrivalTime = generateBusArrivalTime(prevStopTime, busScheduleDifferences[stopNumber - 1], scheduledArrivalTime);
                if (generatedArrivalTime > scheduledArrivalTime)
                {
                    ++numberOfLateBuses;
                }
                return generatedArrivalTime;
            });

            numLateBuses.push_back(numberOfLateBuses);
        }

        for (std::vector<float>& stopArrivalTimes : busArrivalTimes)
        {
            std::ranges::sort(stopArrivalTimes);
        }
    }
}

int main()
{
    busScheduleCreation::generateBusScheduleIntervals(busScheduleCreation::busSchedule);
    // std::cout << "BusScheduleIntervals: " << std::endl;
    //
    // for (float scheduleInterval : busScheduleCreation::busSchedule)
    // {
    //     std::cout << scheduleInterval << ", ";
    // }
    // std::cout << "\n\nBusArrivalTimes\n\n";

    std::vector<std::vector<float>> busArrivalTimes;
    std::vector<size_t> numberOfLateBusesPerStop;

    bus::generateBusArrivalTimes(busArrivalTimes, numberOfLateBusesPerStop, busScheduleCreation::busSchedule);

    std::string copyString;
    size_t digits = std::formatted_size("{}", busScheduleCreation::NUM_STOPS);
    copyString.reserve((digits + 1 + StopStats::STATS_FORMAT_LENGTH) * busScheduleCreation::NUM_STOPS);

    for (size_t stopNumber = 0; stopNumber < busArrivalTimes.size(); ++stopNumber)
    {
        // std::cout << "Stats for stop " << stopNumber << "\n";
        // for (float stopArrivalTime : busArrivalTimes[stopNumber])
        // {
        //     std::cout << stopArrivalTime << ", ";
        // }
        // std::cout << "\n";

        copyString.append(std::to_string(stopNumber));
        copyString.append("\t");

        getStopStats(busArrivalTimes[stopNumber], numberOfLateBusesPerStop[stopNumber], bus::BUS_INTERVAL).appendStatsStringTo(copyString);
        // std::cout << "\n\n";
    }

    std::cout << "Data copied to clipboard, paste into desmos or g sheets for visualization" << std::endl;
    copyToClipboard(copyString);
}