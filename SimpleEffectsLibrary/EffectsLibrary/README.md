# Simple Effects shader library

This folder contains the complete 93-effect collection, organized in the same
categories shown by Oceanode's New Node menu.

Copy the category folders and `SimpleEffectCommon.inc` into the application's
`bin/data/Effects/` directory while preserving the folder structure. The
`.inc` file is shared implementation and is not registered as a node.

User-connected secondary textures use normalized coordinates and stretch to the
primary `tSource` canvas. Effects that require these textures return `tSource`
unchanged while they are disconnected. Optical Flow instead uses automatic
input history and emits neutral motion until its previous frame is available.
Feedback uses automatic previous-output storage and seeds its first frame from
the current input.

## Color and tone

- `Exposure`, `BrightnessContrast`, `LevelsPro`, `LiftGammaGain`, `HSVAdjust`
- `Vibrance`, `TemperatureTint`, `Tint`, `Tritone`, `ChannelMixer`
- `SelectiveChannel`, `Posterize`, `Solarize`, `ClampRange`

## Keying, matte, and alpha

- `ChromaKey`, `DifferenceKey`, `InvertedLumaKey`, `LumaRangeKey`
- `LumaKey`, `Minimax`, `AlphaFromChannel`, `MatteChoker`, `MatteBlur`
- `AlphaPremultiply`, `AlphaUnpremultiply`

## Compositing and multi-texture processing

- `Crossfade`, `BlendAdd`, `BlendMultiply`, `BlendScreen`, `BlendOverlay`
- `BlendSoftLight`, `BlendDifference`, `BlendExclusion`, `BlendMin`, `BlendMax`
- `MaskedBlend`, `SetMatte`, `SetColorKeepAlpha`, `InjectAlpha`, `LightWrap`
- `GradientMap`, `LUT2D`, `TextureRemap`

## Transform and distortion

- `Transform2D`, `CropFeather`, `Mirror`, `TileOffset`, `Kaleidoscope`
- `Twirl`, `BulgePinch`, `WaveWarp`, `Ripple`, `LensDistortion`
- `CornerPin`, `ChromaticAberration`, `DisplacementMap`, `VectorWarp`

## Blur and detail

- `BoxBlur`, `GaussianBlur`, `DirectionalBlur`, `RadialBlur`, `ZoomBlur`
- `Sharpen`, `UnsharpMask`, `HighPass`, `SobelEdge`, `Emboss`, `Outline`

## Stylize

- `Pixelate`, `Halftone`, `Dither`, `Grain`, `Scanlines`, `RGBSplit`
- `BadTVGlitch`, `QuantizeColor`, `DuotonePosterize`, `Vignette`
- `DropShadow`, `InnerShadow`, `GlowExtract`, `FilmLook`

## Motion analysis

- `OpticalFlow`: dense, single-scale Lucas-Kanade motion estimation with
  automatic previous-input storage. Appears at
  `Modules/Effects/Motion analysis/OpticalFlow`.

Connect a camera/video texture to `Input`; no external buffer is necessary.
Start with `Texture Resizer` upstream at roughly 320x180, `WindowRadius = 2`,
and `OutputMode = 1` to inspect direction and speed. The default 5x5 window
uses 250 luminance texture reads per interior output pixel; increase resolution
or radius only after measuring performance. Larger radii increase cost and
blend motion across object boundaries.

| OutputMode | Output | Encoding |
| --- | --- | --- |
| 0 (default) | Raw Flow | R = signed X displacement, G = signed Y displacement, B = confidence, A = 1 |
| 1 | Direction Colour | Hue = direction; brightness = speed / DisplayScale |
| 2 | Magnitude | Grayscale speed / DisplayScale, clamped to 0..1 |
| 3 | Motion Mask | White for motion surviving the confidence and motion thresholds; black otherwise |
| 4 | Confidence | Grayscale local texture confidence, independent of Gain and motion gating |
| 5 | Vector Overlay | Source image with a grid of direction-coloured arrows |

Only the selected mode is emitted through the single `Output` socket. With
`Gain = 1`, raw vectors describe forward motion from the previous image to the
current image in **source pixels per processed Input update**. Positive X/Y
follow increasing texture U/V, before display flips. `MaxMotion` caps vector
length before `Gain`; `MotionThreshold` also uses pre-gain pixels.
Data modes use alpha 1, while Vector Overlay preserves source alpha.

For distortion, connect Raw Flow to `VectorWarp.VectorMap`, set `Center = 0`,
and start with `AmountX = AmountY = 1` at matching resolutions. VectorWarp
samples at `uv + displacement`; use negative amounts to move visible features
along the measured forward direction. If the warped image is larger than the
analysis image, multiply AmountX/Y by the respective width/height ratios.
Raw vectors can also drive a future fluid or particle node; this shader does
not implement fluid simulation.

`Regularization` stabilizes the solve. `ConfidenceThreshold` suppresses regions
without enough two-dimensional texture, and `MotionThreshold` removes weak
motion. Flat regions and isolated edges cannot reliably determine both motion
components. Confidence measures local texture conditioning, not the probability
that a vector is correct. The estimator assumes approximately constant
brightness, RGB in 0..1, and small inter-frame motion; lighting changes,
occlusion, and large/fast motion can produce wrong vectors. Raising `MaxMotion`
does not add a coarse-to-fine search.

The first input after a reset yields zero motion and confidence. History advances
on Input notifications, not on slider changes or redraws, so a slow video source
keeps its latest flow between updates. With `Draw On Event` off, notifications
between draws are coalesced; turn it on to process each notification immediately.
Use `Reset History` after seeking or changing content within the same source
node. Reconnection, resizing, bypass changes, successful reload, and deactivation
reset history automatically. Optical Flow uses input history; recursive image
feedback uses the separate previous-output support described below.

## Temporal

- `Feedback`: recursive previous-output accumulation with fading, zoom,
  rotation, translation, inversion, and Mix / Lighten / Add modes. Appears at
  `Modules/Effects/Temporal/Feedback`.

### Install and connect a camera

1. Rebuild your Oceanode application with the updated `src/simpleEffect.h`.
2. Copy `Temporal/Feedback.glsl` into the application's
   `bin/data/Effects/Temporal/Feedback.glsl`. Ensure `SimpleEffectCommon.inc`
   exists directly in `bin/data/Effects/`. Copying the complete library while
   preserving its folders also works. Restart the application to register the
   new node; `Reload Shader` alone does not discover new files.
3. Use your application's existing camera capture node/source, select its camera,
   and enable capture. Connect its texture output to `Texture Resizer.Input`.
   Set the resizer to `Width = 640`, `Height = 360` for a 16:9 camera, or
   `640 x 480` for a 4:3 camera. The resizer supplies a `GL_TEXTURE_2D` texture.
   If your application includes `ofxOceanodeNDI`, you can use `NDI Camera`:
   select the sender in `Device` and connect its `Output` to the resizer.
4. Connect `Texture Resizer.Output` to `Feedback.Input`, then `Feedback.Output`
   to the left input pin of `Texture Display` (`Texture In`). A second display
   on the resizer's output makes the live/processed comparison easy.
5. In Feedback's inspector, leave `Draw On Event = false`, keep `Bypass = false`,
   and check `Shader Valid = true` and `Shader Status = OK`. `Reset History` is
   on the node directly after `Bypass`. There is no history socket to wire and
   no need to connect Output back to Input.

```text
Camera texture -> Texture Resizer -> Feedback.Input
                        |               |
                        v               v
                 Texture Display   Texture Display
                    (live)           (feedback)
```

### Example: expanding, rotating light trails

The shader's defaults leave the input unchanged: `Persistence = 0`,
`InputGain = 1`, `Zoom = 1`, `Rotation = 0`, zero offsets, `BlendMode = 0`,
and `InvertHistory = 0`. Feedback renders into its own texture, but the output
pixels retain the source RGBA values, including HDR or signed channels.

To create expanding, rotating trails, use a dark background and move a bright
hand, object, or small light in front of the camera. Set:

| Parameter | Value | Effect |
| --- | --- | --- |
| Persistence | 0.96 | Retains 96% of the previous result each render |
| InputGain | 1 | Fresh camera image at normal brightness |
| Zoom | 1.01 | Previous image expands by 1% per render |
| Rotation | 0.3 | Previous image rotates by 0.3 degrees per render |
| OffsetX / OffsetY | 0 / 0 | No translation |
| BlendMode | 1 | Lighten: retains the brighter of input and fading history |
| BorderMode | 0 | Transparent black outside the transformed history |
| InvertHistory | 0 | Ordinary feedback without colour inversion |

The moving bright object should leave fading trails that expand and curl.
At 60 renders/second, `Persistence = 0.96` gives approximately a 0.28-second
half-life before accounting for transforms; at 30 renders/second it is about
0.57 seconds. Try `0.985` for longer trails. Set `Rotation = 0` and `Zoom = 1`
for simple afterimages, or add `OffsetX = 0.002` for horizontal drift.

For soft full-image ghosts, use `BlendMode = 0` (Mix), `Persistence = 0.90`,
`Zoom = 1`, and `Rotation = 0`. Mix computes
`current * (1 - Persistence) + previous * Persistence`; large persistence also
reduces how strongly new imagery enters. For luminous accumulation, try
`BlendMode = 2` (Add), `InputGain = 0.08`, and `Persistence = 0.94`. Add can
quickly saturate bright areas. When feedback is active, the final blended result
is clamped to 0..1; zero-persistence passthrough and first-frame seeding preserve
the input's full RGBA range.

Lighten computes a per-channel maximum, so bright stationary backgrounds can
hide trails. Add sums RGB and takes the maximum of current and decayed history
alpha; Mix and Lighten apply their respective operation to RGBA. InputGain
affects only fresh RGB. All transforms affect history only, around the centre
in pixel space; Zoom preserves aspect ratio. Offset values are fractions of
canvas width/height. Vertical and rotation directions follow texture coordinates
and can appear reversed by display flips. `BorderMode = 1` wraps the history.

### Example: invert inside the loop

A separate `Solarize` node after Feedback only changes what you see; Feedback
would still store its own result. Set `InvertHistory` to invert the sampled
previous output **before** Feedback blends and stores the next result. A value
between zero and one partially inverts it. Alpha stays unchanged, and
transparent transformed borders stay black.

For a pure white/black loop, connect an opaque white texture to `Feedback.Input`.
Use `Persistence = 1`, `BlendMode = 0` (Mix), `InvertHistory = 1`, `Zoom = 1`,
`Rotation = 0`, `OffsetX = 0`, and `OffsetY = 0`. Set `InputGain = 1` and leave
`Draw On Event` off. Press `Reset History`: the first render seeds white, the
second is black, the third white, and so on. Mix at persistence one ignores
the input after the seed. The switching happens every application draw and can
look grey or flicker at normal frame rates. For a camera rather than a white
source, Reset History seeds from the current camera frame, then flips that
frozen image's colours on alternate renders.

For an evolving inverted trail, start with `InvertHistory = 1`,
`Persistence = 0.9`, `BlendMode = 0`, and a live camera input. New camera imagery
enters on every render while the stored image is inverted before blending.
With `BlendMode = 1` or `2`, the inversion is also applied before Lighten/Add,
but those blend modes need not alternate between exact negatives.

If you add `InvertHistory` to an existing installed Feedback shader, copy this
updated GLSL to `bin/data/Effects/Temporal/Feedback.glsl` and restart the
application: node parameters come from the first metadata line at registration,
so `Reload Shader` alone cannot add the new control.

### Manual verification after rebuilding

- With the new defaults (`Persistence = 0`, `InputGain = 1`), output should match
  the input exactly, including alpha and values outside 0..1. Transform and
  invert settings should have no visible effect until persistence is raised.
- Restore the light-trail example and move a bright object: old positions should
  survive across many frames, rather than only the previous camera frame.
- Set `InputGain = 0`: existing RGB trails should continue transforming and
  fade to black. Restore it to 1 to inject the camera again.
- In Mix mode with `Persistence = 1`, `Zoom = 1`, `Rotation = 0`, and zero
  offsets, press `Reset History`: the current camera frame should seed a frozen
  image. Moving the camera afterwards should not change it. Restore persistence
  below 1 to blend new frames again.
- Press `Reset History` with normal settings: accumulated trails disappear and
  the next render starts with the current input. Toggle Bypass on then off, reconnect
  Input, change resizer dimensions, or successfully reload the shader to check
  the same reset behavior. Disconnecting input should clear the output.
- Add a second Feedback instance with different settings: histories should be
  independent. Optical Flow should continue comparing input snapshots as before.

These are runtime checks for the rebuilt application; source and numerical
validation alone do not establish GPU correctness or performance.

## Patchable feedback loop

`Texture Feedback Delay` is a separate C++ node in `Modules/Textures`. It lets
you place any processing nodes between its `Output` and `Return` sockets. The
delay copies the returned texture into a separate RGBA32F buffer and publishes
that image on the next application update. It never republishes directly from
its Return callback, which prevents the patch cable from creating immediate
recursive computation. Its `Output` texture pointer stays stable as the two
internal buffers alternate.

Rebuild and restart your application with the updated addon to register the
node. No new GLSL installation is needed for the delay node itself. Individual
shaders such as `Solarize` still need to be installed under `bin/data/Effects/`.

For the white/invert example:

```text
Color Texture.Output (white) ---> Texture Feedback Delay.Seed
Texture Feedback Delay.Output ---> Solarize.Input
Solarize.Output -------------> Texture Feedback Delay.Return
Texture Feedback Delay.Output ---> Texture Display.Texture In
```

Set `Color Texture` to an opaque white colour at the desired resolution, for
example `640 x 360`. Set `Solarize.Threshold = 0`, `Softness = 0`, and `Mix = 1`
for complete RGB inversion. Set Solarize's inspector `Draw On Event = true` so
it processes the new delayed image as soon as it is published. Leave the delay
node's `Run` on, then press `Reset`. The first published image is the white
seed; each returned image becomes the next iteration: white, black, white, and
so on. The direct branch to Texture Display shows the **stored** loop image.
Connect the display to Solarize instead if you want to see the inverted image
going into `Return`. At normal frame rates the alternating image can appear
grey or flicker.

Replace Solarize with a chain such as `Solarize -> Transform2D -> GlowExtract`,
or insert compositing nodes fed by a camera. Connect only the last node's
output to `Return`. Set `Draw On Event = true` on every Simple Effect in the
chain for immediate propagation. Nodes that calculate only during their own
`draw()` may add one or more frames of latency depending on the graph's draw
order; the delay still prevents immediate recursion. Inspect their output with
Texture Display if the loop appears to stall. The returned texture's resolution
becomes the next iteration's resolution, so put a Texture Resizer in the chain
if you want to keep a fixed canvas size. Seed and Return must be `GL_TEXTURE_2D`.

`Run = false` holds the current output. While paused, `Step` sends that image
through the chain once; press `Step` again to publish the returned result.
`Reset` discards the stored images and copies the current Seed on the next
update. `Iteration` in the inspector counts returned images that have been
published. The Seed is used again only after Reset or node deactivation.

The original `Feedback.glsl` remains useful for a compact all-in-one effect.
Use `Texture Feedback Delay` when you need to edit the loop with patch cables.

## Transitions

- `LinearWipe`, `RadialWipe`, `IrisTransition`, `LumaDissolve`
- `DisplacementTransition`, `NoiseDissolve`, `PixelSortTransition`
- `RGBSplitTransition`, `BurnLightTransition`

## Parameter conventions

Every shader begins with SimpleEffect metadata, then provides `@description`
and `@param` comments used by the node Description field.

The relative folder becomes the registration category. For example,
`Transform and distortion/Transform2D.glsl` appears at
`Modules/Effects/Transform and distortion/Transform2D`.

Continuous float controls normally declare optional `NameTex` and
`NameTexConnected` uniforms. A connected control texture is interpreted as a
normalized zero-to-one value and mapped into the parameter's documented range.
Discrete selectors and sampling-count controls deliberately remain scalar to
avoid unstable per-pixel branching and variable rendering cost.
Optical Flow also keeps `Regularization` scalar for a consistent solver baseline.
