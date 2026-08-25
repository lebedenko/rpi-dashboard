# OpenGL vs Vulkan benchmark: aborted study

## Verdict

Keep OpenGL on the current Raspberry Pi production stack. Vulkan hardware rendering initializes and runs, but the application crashes during normal Ctrl+Q shutdown under Cage/Wayland. Because no Vulkan trace is flushed, the four-run ABBA study cannot satisfy its clean-exit requirement and no frame-performance claim can be made for Vulkan.

## Environment

| Component | Version |
|---|---|
| Raspberry Pi kernel | 6.18.39+rpt-rpi-2712 |
| Qt | 6.8.2 |
| Mesa Vulkan drivers | 26.2.0-1~bpo13+0~rpt3 |
| Wayland client | 1.24.0-2~bpo13+1~rpt1 |
| Cage | 0.2.0 |
| Display | 320x1480 at 57.68 Hz, transform 270, logical 1480x320 |

## Captures

| Run | Backend | Hardware identity | Wall time | Temperature | Exit | Trace |
|---|---|---|---:|---:|---|---|
| 1 | OpenGL | Broadcom V3D 7.1.10.2 | 90.88 s | 57.9 C start, 59.5 C max | 0, accepted | 1,001,529 bytes |
| 2 | Vulkan | V3D 7.1.10.2, Mesa V3DV, explicitly selected by Qt | 90.65 s | 56.2 C start, 56.8 C max | 3, crashed | none |
| 3 | Vulkan | Not run after mandatory failure | - | - | - | - |
| 4 | OpenGL | Not run after mandatory failure | - | - | - | - |

The backend starts were thermally comparable: run 2 began 1.7 C below run 1, within the required 2 C range. Temperature does not explain the Vulkan failure.

## Failure evidence

Qt successfully created its Vulkan QRhi, selected physical device 0 (`V3D 7.1.10.2`, vendor `0x14E4`), and created a four-buffer 1480x320 swapchain. At normal window shutdown, the Wayland client reported:

```text
warning: queue "mesa vk display queue" ... destroyed while proxies still attached
Proxy and queue point to different wl_displays
Error: Process crashed!
```

The same failure occurred in two independent Vulkan attempts, before and after changing Ctrl+Q from an application-wide quit to orderly `ApplicationWindow.close()`. In both attempts qmlprofiler exited with status 3 and did not produce a `.qtd` file.

Cage also printed `A configure is scheduled for an uninitialized xdg_surface` during both OpenGL and Vulkan runs. Since OpenGL exits normally and writes a valid trace, that compositor warning is not sufficient to explain the backend-specific crash.

## Recommendation

Retain OpenGL. Vulkan is not production-viable on this exact Qt/Mesa/Cage/Wayland combination because normal shutdown is unreliable. Revisit Vulkan only after a relevant Qt, Mesa, Wayland, or Cage update, beginning with a short launch-and-Ctrl+Q smoke test before repeating the full ABBA benchmark.

> AI assistance has been used to create this output.
