cmake_minimum_required(VERSION 4.1)
project(BusWaitTimeSimulation)

set(CMAKE_CXX_STANDARD 26)

include_directories("C:/Users/The 64th Realm/Documents/Libraries/cpp/boost_1_90_0")
add_executable(SingleStop singleStopSimulation.cpp
        rngInitializer.h
        waitStats.h
)
add_executable(MultipleStops multipleStopSimulation.cpp
        rngInitializer.h
        waitStats.h
        windowsClipboardHelper.h
)
