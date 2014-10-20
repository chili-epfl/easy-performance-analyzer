easy-performance-analyzer
=========================

A simple cross-platform instrumented performance analysis tool:

  - Linux and Android support
  - Instrumentation calls take ~1 microsecond on tested machines
  - Real-time analysis and reporting option
  - Smoothing over time option
  - Offline aggregation option (like regular profiling)
  - Multithreading support
  - On-demand enable/disable support

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

You'll need the **Android SDK** (tested with Android platform version 14, i.e Android 4.0) and **Android NDK** (tested with r9d).

These instructions assume `armv7-a` target architecture. For other architectures, adapt the instructions to your liking.

1. Export a standalone NDK toolchain:

  ```
  cd /path-to-android-ndk
  ./build/tools/make-standalone-toolchain.sh \
      --platform=android-14 \
      --install-dir=/desired-path-to-android-standalone-toolchain \
      --toolchain=arm-linux-androideabi-4.8
  ```

  Be aware that if you don't have write access to `/desired/path/to/android/standalone/toolchain`, the script fails silently.

2. Set up the following environment variables:

  ```
  export ANDROID_HOME=/path-to-android-sdk/
  export ANDROID_SDK_ROOT=$ANDROID_HOME
  export ANDROID_SDK=$ANDROID_SDK_ROOT
  export ANDROID_NDK_ROOT=/path-to-android-ndk/
  export ANDROID_NDK_STANDALONE_TOOLCHAIN=/path-to-android-standalone-toolchain/
  export ANDROID_STANDALONE_TOOLCHAIN=$ANDROID_NDK_STANDALONE_TOOLCHAIN
  export ANDROID_ABI=armeabi-v7a
  export ANDROID_NATIVE_API_LEVEL=14
  export ANDROID_TOOLCHAIN_NAME=arm-linux-androideabi-4.8
  ```

  Make sure that the `ANDROID_NDK` environment variable is **not set** before proceeding.

3. In the `easy-performance-analyzer` source directory:

  ```
  mkdir build-android
  cd build-android
  cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/android.toolchain.cmake
  make -j 5
  make install
  ```

Now `easy-performance-analyzer` is installed in your standalone toolchain under `/path-to-android-standalone-toolchain/sysroot/usr/` just like a regular library. To build an example user program, execute:

```
/path-to-android-standalone-toolchain/bin/arm-linux-androideabi-g++ input.cpp -o example-program -lezp -llog
```

Please note that you have to link `log` yourself when compiling if it's not automatically being linked by your build system. See the samples for more details.

How to use
----------

There are 3 ways to use easy-performance-analyzer:

  1. **Real-time analysis**

    Execution time of a block of code is measured and printed in real time. Example usage:

    ```
    #include<ezp.hpp>

    ...

        EZP_ENABLE

    ...

        EZP_START("XMPL")

        ... your code ...

        EZP_END("XMPL")

    ...
    ```

    The time it took to run the block between the macros is measured and printed as soon as `EZP_END()` is called.

  2. **Smoothed real-time analysis**

    Execution time of a block of code is measured and printed in real time, and is smoothed in time with an IIR low pass filter
    using past measurements. Of course, this is only useful for scenarios where the block is called periodically where you want to see the average
    time it takes to run. Example usage:

    ```
    #include<ezp.hpp>

    ...

        EZP_ENABLE

    ...

        forever{
            EZP_START_SMOOTH("XMPL")

            ... your code ...

            EZP_END_SMOOTH("XMPL")
        }

    ...
    ```

    The time it took to run the block between the macros is measured and printed as soon as `EZP_END_SMOOTH()` is called.

    You can replace `EZP_END_SMOOTH(block_name)` with `EZP_END_SMOOTH_FACTOR(block_name,custom_smoothing_factor)` to force your
    own smoothing coefficient. This coefficient is multiplied with the history and should be between 0 and 1. Closer to 0 means no
    smoothing and closer to 1 means no update at all.

  3. **Offline aggregate analysis**

    Execution time of a block is measured and kept for later use. Example usage:

    ```
    #include<ezp.hpp>

    ...

        EZP_ENABLE

    ...

        forever{
            EZP_START_OFFLINE("XPML")

            ... your code ...

            EZP_END_OFFLINE("XMPL")
        }

    ...

        EZP_PRINT_OFFLINE

    ...
    ```

    Average and total execution times and number of executions of all offline instrumented blocks are printed when `EZP_PRINT_OFFLINE` is called.
    `EZP_CLEAR_OFFLINE` can be called at any time to erase the offline analysis history.

All three methods can be used simultaneously and can be nested. See the samples for more detailed example usage.

**Important note 1**: Block names must be 4 characters maximum: This is for faster instrumentation so that your measurements can be more accurate and the original code is disturbed less.

**Important note 2**: Printing to stdout or Logcat in real time takes significant amount of time (on tested machines, on the order of tens of microseconds); this could disturb yor measurements. For time critical applications, prefer **offline** analysis which will provide the lightest instrumentation.

API Summary
-----------

  - `EZP_ENABLE` - Enables all analysis
  - `EZP_DISABLE` - Disables all analysis
  - `EZP_SET_ANDROID_TAG(TAG)` - Sets the Logcat tag of printed messages
  - `EZP_START(BLOCK_NAME)` - Begins a real-time analysis block
  - `EZP_END(BLOCK_NAME)` - Ends a real-time analysis block and prints execution time
  - `EZP_START_SMOOTH(BLOCK_NAME)`- Starts a smoothed real-time analysis block
  - `EZP_END_SMOOTH(BLOCK_NAME)` - Ends a smoothed real-time analysis block and prints smoothed execution time using the history of measurements
  - `EZP_END_SMOOTH_FACTOR(BLOCK_NAME,SMOOTHING_FACTOR)` - Ends a smoothed real-time analysis block and prints the smoothed execution time with custom smoothing factor
  - `EZP_START_OFFLINE(BLOCK_NAME)` - Starts an offline analysis block
  - `EZP_END_OFFLINE(BLOCK_NAME)` - Ends an offline analysis block
  - `EZP_PRINT_OFFLINE` - Prints average and total times and numbers of execution of all offline analysis blocks
  - `EZP_CLEAR_OFFLINE` - Erases the offline analysis history

In all calls, `BLOCK_NAME` is maximum 4 characters long.

Samples
-------

To build samples, enable `WITH_SAMPLES` during build. See [samples/README.md](samples/README.md) for more details.

