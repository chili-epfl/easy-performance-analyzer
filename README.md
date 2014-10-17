easy-profiler
=============

Just the simple cross-platform profiling tool for C++ you've been looking for!

  - Supports Linux and Android
  - Real-time profiling and reporting option
  - Smoothing over time option
  - Offline aggregation option (like regular profiling)
  - Multithreading support (coming soon)

Linux Build
-----------

```
mkdir build-desktop
cd build-desktop
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
make -j 5
make install
```

Android Build
-------------

Coming soon...

How to use
----------

There are 3 ways to use easy-profiler:

  1. **Real-time profiling**

    Execution time of a block of code is measured and printed in real time. Example usage:

    ```
    #include<EasyProfiler.hpp>

    ...

        EZP_START("Example block")

        ... your code ...

        EZP_END("Example block")

    ...
    ```

    The time it took to run the block between the macros is measured and printed as soon as `EZP_END()` is called.

  2. **Smoothed real-time profiling**

    Execution time of a block of code is measured and printed in real time, and is smoothed in time with an IIR low pass filter
    using past measurements. Of course, this is only useful for scenarios where the block is called periodically where you want to see the average
    time it takes to run. Example usage:

    ```
    #include<EasyProfiler.hpp>

    ...

        forever{
            EZP_START_SMOOTH("Example block")

            ... your code ...

            EZP_END_SMOOTH("Example block")
        }

    ...
    ```

    The time it took to run the block between the macros is measured and printed as soon as `EZP_END_SMOOTH()` is called.

    You can replace `EZP_END_SMOOTH(block_name)` with `EZP_END_SMOOTH_FACTOR(block_name,custom_smoothing_factor)` to force your
    own smoothing coefficient. This coefficient is multiplied with the history and should be between 0 and 1. Closer to 0 means no
    smoothing and closer to 1 means no update at all.

  3. **Offline aggregate profiling**

    Execution time of a block is measured and kept for later use. Example usage:

    ```
    #include<EasyProfiler.hpp>

    ...

        forever{
            EZP_START_OFFLINE("Example block")

            ... your code ...

            EZP_END_OFFLINE("Example block")
        }

    ...

        EZP_PRINT_OFFLINE

    ...
    ```

    Average and total execution times and number of executions of all offline profiles are printed when `EZP_PRINT_OFFLINE` is called.
    `EZP_CLEAR_OFFLINE` can be called at any time to erase the offline profile history.

All three methods can be used simultaneously and can be nested. See the samples for more detailed example usage.

API Summary
-----------

  - `EZP_SET_ANDROID_TAG(TAG)` - Sets the Logcat tag of printed messages
  - `EZP_START(BLOCK_NAME)` - Starts a real-time profile
  - `EZP_END(BLOCK_NAME)` - Ends a real-time profile and prints execution time
  - `EZP_START_SMOOTH(BLOCK_NAME)`- Starts a smoothed real-time profile
  - `EZP_END_SMOOTH(BLOCK_NAME)` - Ends a smoothed real-time profile and prints smoothed execution time using the history of measurements
  - `EZP_END_SMOOTH_FACTOR(BLOCK_NAME,SMOOTHING_FACTOR)` - Ends a smoothed real-time profile and prints the smoothed execution time with custom smoothing factor
  - `EZP_START_OFFLINE(BLOCK_NAME)` - Starts an offline profile
  - `EZP_END_OFFLINE(BLOCK_NAME)` - Ends an offline profile
  - `EZP_PRINT_OFFLINE` - Prints average and total times and numbers of execution of all offline profiles
  - `EZP_CLEAR_OFFLINE` - Erases the offline profile history

Samples
-------

To build samples, enable `WITH_SAMPLES` during build. See [samples/README.md](samples/README.md) for more details.

