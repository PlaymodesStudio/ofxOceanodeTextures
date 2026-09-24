# Simple Effect shader contract

Copy `SimpleEffectsLibrary/SimpleEffectTemplate.glsl` into an application's
`bin/data/Effects/` directory or one of its category subfolders, then rename it
to create a runtime effect node. Effects are discovered recursively when the
Oceanode models are registered. A relative folder such as
`Effects/Transform and distortion/` becomes the New Node category
`Modules/Effects/Transform and distortion/`. Use the node's `Reload Shader`
inspector control after editing GLSL; restart the application when adding,
removing, moving, or renaming effect files.

## First-line metadata

The first line declares node parameters. Whitespace around commas and colons is
accepted.

```glsl
//Amount:1:0:2, Tint:color:1:1:1:1, Mask:texture
```

- `Name` creates an unbounded float with default `0`.
- `Name:default` creates an unbounded float.
- `Name:default:min:max` creates a ranged float. `min` and `max` can be used as
  open-ended range markers.
- `Name:color` creates an `ofFloatColor` parameter bound to `uniform vec4 Name`.
- `Name:color:R:G:B` and `Name:color:R:G:B:A` set its default color.
- `Name:texture` creates a texture parameter bound to `uniform sampler2D Name`.

Names are GLSL identifiers and are case-sensitive. A metadata float named
`Amount` therefore requires `uniform float Amount;`, not `amount`.

## Description annotations

Optional shader comments populate Oceanode's existing node Description field:

```glsl
// @description Applies a concise explanation of the effect.
// @param Amount: Controls the strength of the effect.
// @param Mask: Optional texture limiting where the effect is applied.
```

Multiple `@description` lines are joined into a paragraph. Every `@param` line
is displayed on a separate line under a `Parameters:` heading. Annotations can
be changed while developing and are refreshed by `Reload Shader`.

## Automatically supplied uniforms

```glsl
uniform sampler2D tSource;
uniform vec2 uResolution;
uniform vec2 uTexelSize;
uniform float uTime;
uniform int uFrame;
```

The legacy aliases `resolution`, `time`, and `frame` are also supplied. Animated
effects should leave `Draw On Event` disabled so they render continuously.

For a float named `Amount`, simpleEffect optionally supplies:

```glsl
uniform sampler2D AmountTex;
uniform int AmountTexConnected;
```

For an explicit texture named `Mask`, it supplies:

```glsl
uniform sampler2D Mask;
uniform int MaskConnected;
```

Only sample an optional texture when its connection flag is non-zero. This
allows legitimate 1x1 textures and avoids using texture size as a connection
sentinel.

## Output conventions

- The output is an RGBA32F texture matching `tSource` dimensions.
- Use `gl_FragCoord.xy / uResolution` for normalized sampling coordinates.
- Use `texelFetch(tSource, ivec2(gl_FragCoord.xy), 0)` for exact pixel access.
- Preserve `source.a` by default. Change alpha only when it is part of the
  effect's purpose.
- Protect zero denominators and invalid `smoothstep` ranges with an epsilon.
- Failed reloads keep the previous valid shader active and report the problem
  through `Shader Status` and the application log.

## Optional input history

A shader that samples the following reserved uniform automatically enables
input history; do not add it to the first-line parameter metadata:

```glsl
uniform sampler2D tPreviousSource;
uniform int tPreviousSourceConnected;
```

`tSource` and `tPreviousSource` then contain an immutable pair of current and
previous input snapshots, both RGBA32F `GL_TEXTURE_2D` textures. This costs two
additional textures and one GPU copy per captured input update. Shaders without
an active `tPreviousSource` uniform do not allocate or copy history.

Input parameter notifications advance history, including notifications that
reuse the same texture pointer. Redraws and changes to effect controls reuse the
same pair. Producers must notify Input after updating texture pixels; silently
mutating a texture cannot be detected. With `Draw On Event` disabled, multiple
input notifications before a draw are coalesced into the latest snapshot. Enable
`Draw On Event` to process each notification immediately. The first snapshot may
also be taken on initial compute or after a reset.

`tPreviousSourceConnected` is zero until two snapshots exist. Return a neutral
result while it is zero; the fallback sampler is a 1x1 black texture, so do not
use source-sized `texelFetch` coordinates on it. History resets on input
connection changes, missing input, resolution changes, bypass changes,
deactivation, successful shader reload, and `Clear`. The button appears on the
node directly after Bypass only when the compiled shader actively uses
`tPreviousSource` or `tPreviousOutput`. Ordinary effects have no history button.
Use it after a seek or a content switch inside the same source node. Failed
shader reloads preserve the active shader and its history.

History effects require `GL_TEXTURE_2D` inputs and render with blending disabled
to preserve numerical channels and alpha exactly. They retain the latest pair
when no new input is notified; `uTime` can still animate their presentation.
This is **input history**. Recursive output feedback is a separate opt-in below.

`Motion analysis/OpticalFlow.glsl` demonstrates this contract. Its raw RG output
is forward displacement in source pixels per processed input update, rather
than pixels per second. See the [shader catalog](SimpleEffectsLibrary/EffectsLibrary/README.md#motion-analysis)
for modes and patching examples.

## Optional output feedback

A shader that samples this reserved uniform automatically enables recursive
previous-output storage; do not put either uniform in the metadata line:

```glsl
uniform sampler2D tPreviousOutput;
uniform int tPreviousOutputConnected;
```

`tPreviousOutput` contains the result of the last completed render of this node.
The host alternates between two RGBA32F `GL_TEXTURE_2D` output buffers, reading
one while writing the other. This uses one additional full-size texture compared
with an ordinary effect, with no extra GPU copy pass. The published `ofTexture*`
has a stable address even as its underlying GPU texture alternates. Output
buffers use nearest filtering; shaders can implement bilinear sampling, as
`Temporal/Feedback.glsl` does, when smooth subpixel transforms are needed.

On the first render after reset, `tPreviousOutputConnected` is zero and the
sampler points to the 1x1 transparent-black fallback. Seed the result from
`tSource`, or return another intentional initial state, before sampling history.
The flag becomes one on the next render. Both input and output history require
`GL_TEXTURE_2D` inputs and render with alpha blending disabled.

Output history advances on **every compute/render**, independently of input
notifications. With `Draw On Event` off, it evolves once per application draw,
including with an unchanged image. With it on, input notifications and parameter
or control-texture changes each request a render and advance feedback. Feedback
speed and decay are per render, not per second or necessarily per camera frame.
Do not enable event rendering expecting changes to sliders to be history-neutral.

`Clear` clears both histories. Input reconnection/disconnection, invalid
input, resolution changes, bypass changes, deactivation, and successful shader
reload also reset them. Failed reloads retain the active shader and its history.
Successful reloads add, remove, or relocate the button on the next update if
the set of active history uniforms changes.

`tPreviousSource` and `tPreviousOutput` can be used together: the former is the
previous captured input, the latter is the previous processed output. Each
reserves its own texture unit ahead of effect/control textures. Shaders that
do not sample `tPreviousOutput` allocate no feedback buffers. History is internal
to each node instance; no output-to-input patch cable is necessary.

See the [Feedback camera example and test checklist](SimpleEffectsLibrary/EffectsLibrary/README.md#temporal).
