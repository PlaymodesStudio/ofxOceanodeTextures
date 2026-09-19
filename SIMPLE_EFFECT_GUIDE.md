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
