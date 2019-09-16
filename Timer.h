#ifndef TIMER_H_
#define TIMER_H_

#include <sys/time.h>

/*
    Timer struct
*/
#if defined _WIN32 || defined _WIN64 //Windows timer
    #include <io.h>
    #include <windows.h>

    struct Timer
    {
        clock_t startTime, currentTime;

        Timer()
        {

        };

        void start()
        {
            startTime = clock();
        };

        //returns how long it has been since the timer was last started in seconds
        double getTime()
        {
            currentTime = clock();

            return (double)(currentTime - startTime)/1000.0;
        };
    };
#else //Mac/Linux Timer
    struct Timer
    {
        timeval timer;
        double startTime, currentTime;

        Timer()
        {

        };

        //starts the timer
        void start()
        {
            gettimeofday(&timer, NULL);
            startTime = timer.tv_sec+(timer.tv_usec/1000000.0);
        };

        //returns how long it has been since the timer was last started in seconds
        double getTime()
        {
            gettimeofday(&timer, NULL);
            currentTime = timer.tv_sec+(timer.tv_usec/1000000.0);
            return currentTime-startTime;
        };
    };
#endif


#endif //TIMER_H_
