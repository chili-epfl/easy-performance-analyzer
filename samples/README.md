easy-performance-analyzer samples
=================================

Here are the current samples:

  - **real-time**: Demonstrates the basic usage of easy-performance-analyzer
  - **offline**: Demonstrates the basic offline usage of easy-performance-analyzer
  - **compiler-optimization**: Demonstrates the effects of compiler optimization on code speed
  - **multithreaded**: Demonstrates the usage with multiple threads running the same analysis blocks
  - **instrumentation-performance**: Demonstrates the performance of EZP instrumentation calls themselves
  - **external-control**: Demonstrates the usage of `ezp_control`

Linux Build
-----------

Enable `WITH_SAMPLES` during usual build (enabled by default). Samples are not installed, launch them from inside the `samples/bin/` directory in the build root.

Android Build
-------------

Enable `WITH_SAMPLES` during usual build (enabled by default). Samples are not installed, but placed inside the `samples/bin/` directory in the build root. To launch e.g `real-time`, connect your **rooted** device, launch a DDMS instance and execute from within the build root:

```
adb push samples/bin/ /sdcard/ezp-samples/
adb shell
```

You should be in your device's shell now. From here:

```
su
/sdcard/ezp-samples/real-time
```

The output will be printed to logcat in the DDMS instance under the tag `EZP`.

