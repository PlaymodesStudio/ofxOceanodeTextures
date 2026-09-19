# Simple Effects: first-batch validation

Use a small graph with a stable test image, a grayscale ramp, RGB color bars,
an alpha-gradient image, and a second texture with an obviously different
resolution and aspect ratio. Test at both a small resolution and the intended
show resolution.

## Tests for every effect

- Confirm the node appears once, has `Shader Valid = true`, and its Description
  explains every visible control.
- Confirm `Bypass` returns the exact input and preserves alpha.
- Disconnect `Input`; `Output` should become null rather than retain a stale
  frame. Reconnect it and confirm rendering resumes.
- Enable `Draw On Event`; change every scalar, color, and texture input and
  confirm the output updates.
- Connect a grayscale ramp to each texture-capable scalar. Check the effect
  varies spatially and returns to its scalar value after disconnection.
- Test 1x1, portrait, landscape, HD, and odd-numbered resolutions.
- Test transparent pixels, alpha gradients, negative float values, values above
  one, pure black, pure white, and saturated RGB primaries.
- Temporarily introduce a GLSL error and press `Reload Shader`. Confirm the
  previous valid shader stays active and `Shader Status` reports the error.
- Restore the file, reload it, and confirm status returns to `OK`.

## Expected checks by effect

- **LumaKey:** values below Threshold are exactly black; Preview is a grayscale
  matte; zero Softness behaves as a hard threshold; Feather reduces noisy edges.
- **Exposure:** zero is identity; plus one doubles RGB; minus one halves RGB.
- **LevelsPro:** defaults are identity; Gamma never produces NaN; moving black
  and white points compresses the expected range.
- **LiftGammaGain:** defaults are identity; Lift affects the floor, Gain scales
  the signal, Gamma shapes midtones, and Saturation zero is grayscale.
- **HSVAdjust:** Hue one returns to the starting hue; Saturation zero is
  grayscale; Value zero is black; alpha remains unchanged.
- **Tritone:** black, mid-gray, and white approach the three selected colors;
  Mix zero is identity.
- **Transform2D:** defaults are identity; anchor, position, rotation, scale, and
  transparent out-of-bounds areas behave independently.
- **Pixelate:** cell dimensions are measured in pixels; Mix zero is identity;
  odd resolutions do not create uninitialized borders.
- **SobelEdge:** a flat image produces no edges; a black-white boundary produces
  a strong edge; zero Softness remains finite.
- **DirectionalBlur:** Length zero is identity; Samples one is identity; image
  brightness remains stable as Samples changes.
- **ChromaKey:** the selected color becomes transparent/black; unrelated colors
  remain; Preview matches alpha; Despill does not change retained regions.
- **BlendMultiply:** white LayerB is identity; black LayerB produces black;
  LayerB alpha and Mix control the contribution.
- **BlendScreen:** black LayerB is identity; white LayerB produces white;
  LayerB alpha and Mix control the contribution.
- **SetMatte:** white matte preserves the source; black matte removes it;
  UseAlpha, Invert, Amount, and Premultiply are independently visible.
- **GradientMap:** black samples the left edge, white the right edge, and gray
  the middle; disconnected Gradient returns the source.
- **DisplacementMap:** a map value equal to Center is identity; red and green
  move X and Y independently; edge handling is stable.
- **DifferenceKey:** identical source/reference produces an empty matte;
  different images remain; Preview matches output alpha.
- **LinearWipe:** Progress zero is exactly source and one exactly LayerB; Angle
  changes direction; Softness changes only the transition boundary.
- **LumaDissolve:** endpoints are exact; the optional Map controls reveal order;
  Burn Width zero disables the burn edge.

## Acceptance gate before expanding deployment

- No shader reports compile/link errors.
- Default settings are identity where documented.
- No NaN, flashing, stale output, or texture-unit contamination appears.
- Alpha behavior matches each effect's description.
- Secondary-texture behavior is accepted as normalized stretch, or a later
  common fit/crop policy is defined before production use.
- Performance is measured at show resolution, especially DirectionalBlur,
  SobelEdge, LumaKey Feather, and transition effects.

## Minimax validation

- Use a white square and isolated white dots on black, including the same shapes
  in alpha. `Minimum` should contract white regions; `Maximum` should expand
  them.
- `Minimum Then Maximum` should remove small bright details, while `Maximum
  Then Minimum` should fill small dark gaps.
- `Radius = 0` and `BlendWithOriginal = 1` should each return the source.
- Compare Horizontal, Vertical, and Horizontal and Vertical directions using
  thin one-pixel lines aligned to both axes.
- Increase `Samples` until the result is visually stable at the intended show
  resolution. Check performance carefully because combined operations use a
  nested sample set.
