#pragma once

#include <Windows.h>
namespace performance {
    class timestamp_t {
        LARGE_INTEGER StartingTime;
        LARGE_INTEGER FrequencyLI;
        double Frequency;
        mutable LARGE_INTEGER EndingTime;

    public:
        timestamp_t() {
            (void)QueryPerformanceFrequency(&FrequencyLI);
            Frequency = (double)FrequencyLI.QuadPart;
            now();
        }

        inline uint64_t
        frequency() const noexcept {
            return FrequencyLI.QuadPart;
        }

        inline void
        now() noexcept {
            (void)QueryPerformanceCounter(&StartingTime);
        }

        inline double
        sec() const noexcept {
            (void)QueryPerformanceCounter(&EndingTime);
            return (EndingTime.QuadPart - StartingTime.QuadPart) / Frequency;
        }

        inline double
        ms() const noexcept {
            (void)QueryPerformanceCounter(&EndingTime);
            return (EndingTime.QuadPart - StartingTime.QuadPart) * 1E3 / Frequency;
        }

        inline double
        us() const noexcept {
            (void)QueryPerformanceCounter(&EndingTime);
            return (EndingTime.QuadPart - StartingTime.QuadPart) * 1E6 / Frequency;
        }

        // Activity to be timed



        //
        // We now have the elapsed number of ticks, along with the
        // number of ticks-per-second. We use these values
        // to convert to the number of elapsed microseconds.
        // To guard against loss-of-precision, we convert
        // to microseconds *before* dividing by ticks-per-second.
        //

    };

}

