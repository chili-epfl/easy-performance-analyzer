easy-performance-analyzer samples
=================================

Here are the current samples:

  - **real-time**: Demonstrates the basic usage of easy-performance-analyzer
  - **offline**: Demonstrates the basic offline usage of easy-performance-analyzer
  - **compiler-optimization**: Demonstrates the effects of compiler optimization on code speed
  - **multithreaded**: Demonstrates the usage with multiple threads running the same analysis blocks
  - **instrumentation-performance**: Demonstrates the performance of EZP instrumentation calls themselves

Linux Build
-----------

Enable `WITH_SAMPLES` during usual build (enabled by default). Samples are not installed, launch them from inside the `samples/bin/` directory.

Android Build
-------------

Enable `WITH_SAMPLES` during usual build (enabled by default). Samples are not installed, but placed inside the `samples/bin/` directory. To launch e.g `real-time`, connect your **rooted** device, launch a DDMS instance and execute:

```
cd samples/bin/
adb push real-time /sdcard/
adb shell
```

From inside the adb shell, execute:

```
su
/sdcard/real-time
```

The output will be printed to logcat in the DDMS instance under the tag `EZP`.

