# Live2D Core Bridge

This target wraps Live2D Cubism Core for TopiPet's native Metal renderer.

Bundled runtime files:

- `include/Live2DCubismCore.h`
- `lib/macos/arm64/libLive2DCubismCore.a`

These files come from Live2D Cubism SDK for Native 5-r.5 and are governed by the Live2D Proprietary Software License:

- https://www.live2d.com/eula/live2d-proprietary-software-license-agreement_en.html
- https://www.live2d.com/eula/live2d-proprietary-software-license-agreement_cn.html

The local bridge code in `include/Live2DBridge.h` and `src/Live2DBridge.c` loads `.moc3` models, exposes drawable buffers, and lets Swift write Live2D parameters for expressions.
