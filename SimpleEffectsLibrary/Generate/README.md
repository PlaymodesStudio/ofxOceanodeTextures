# Shader library

<!-- oceanode-shaders:start -->
## Shader catalog

- [CircularGradientPro](CircularGradientPro.glsl) — CircularGradientPro generates an aspect-corrected circular radial gradient centered anywhere on the frame and composites it over the incoming texture, or over a transparent black background when UseSource is off. Its principal controls are the gradient center position (X/Y), radius Scale, Color (whose alpha sets peak center opacity), a Pow exponent shaping the falloff from linear to a tight bright center, an Amount blend strength, and an optional Mask texture that limits where the gradient appears. As with other simpleEffect parameters, the scalar controls can each be driven by an external texture connection for animation or modulation.
<!-- oceanode-shaders:end -->
