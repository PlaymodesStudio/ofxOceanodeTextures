# Simple Effects shader library

This folder contains the complete 91-effect collection, organized in the same
categories shown by Oceanode's New Node menu.

Copy the category folders and `SimpleEffectCommon.inc` into the application's
`bin/data/Effects/` directory while preserving the folder structure. The
`.inc` file is shared implementation and is not registered as a node.

All secondary textures use normalized coordinates and stretch to the primary
`tSource` canvas. Effects that require a secondary texture return `tSource`
unchanged while that texture is disconnected.

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
