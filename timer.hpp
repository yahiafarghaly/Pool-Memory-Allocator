#pragma once

#include <chrono>
#include <iostream>

enum TimeUnit
{
        sec,
        ms,
        us,
        ns,
        minutes,
        hr
};

class Timer
{
private:
        std::chrono::high_resolution_clock::time_point t_start;
        std::chrono::high_resolution_clock::time_point t_end;
        TimeUnit timer_unit;

public:
        Timer()
        {
                timer_unit = TimeUnit::ms;
        }

        void setTimeUnit(TimeUnit t)
        {
                timer_unit = t;
        }

        void start()
        {
                t_start = std::chrono::high_resolution_clock::now();
        }

        void end()
        {
                t_end = std::chrono::high_resolution_clock::now();
        }

        /*! \brief return time elapsed in millisecond
         */
        const float getTime()
        {
                switch (timer_unit)
                {
                case sec:
                        return (std::chrono::duration_cast<std::chrono::seconds>(t_end - t_start).count());
                        break;
                case ms:
                        return (std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count());
                        break;
                case us:
                        return (std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count());
                        break;
                case ns:
                        return (std::chrono::duration_cast<std::chrono::nanoseconds>(t_end - t_start).count());
                        break;
                case minutes:
                        return (std::chrono::duration_cast<std::chrono::minutes>(t_end - t_start).count());
                        break;
                case hr:
                        return (std::chrono::duration_cast<std::chrono::hours>(t_end - t_start).count());
                        break;
                default:
                        return (std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count());
                        break;
                };
        }

        const char *getTimeUnitString()
        {
                switch (timer_unit)
                {
                case sec:
                        return " sec";
                        break;
                case ms:
                        return " ms";
                        break;
                case us:
                        return " us";
                        break;
                case ns:
                        return " ns";
                        break;
                case minutes:
                        return " min";
                        break;
                case hr:
                        return " hr";
                        break;
                default:
                        return " ms";
                        break;
                };
        }
};
