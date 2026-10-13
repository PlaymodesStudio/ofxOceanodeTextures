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
- **Flip&Rotate:** all three controls are boolean checkboxes and default to
  false. Test all eight combinations with asymmetric corner labels: FlipX
  reverses source columns, FlipY reverses source rows, then Rotate90 turns
  clockwise. Rotation exchanges width/height without cropping or stretching.
  Every output texel must match a source RGBA texel exactly, including alpha,
  negative values, and values above one. Toggle rotation with Draw On Event
  enabled; dimensions must update immediately. Bypass restores the original
  input pointer and dimensions, and disabling it reapplies the transform.
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

## Shader editor regression

Run `python3 tests/shader_editor_regression.py --syntax --gpu` from this addon on
macOS with access to the desktop GPU. Omit `--gpu` for CPU-only contract checks;
omit `--syntax` when sibling openFrameworks/addon headers are unavailable. The
checks exercise metadata and declaration failures, comments, reserved-name
collisions, import expansion, all library shaders, and actual starter-shader
pixels with scalar/color controls and optional control/mask textures.

Host-app acceptance checks (the automated checks do not launch Oceanode):

1. Add Textures → Shader, connect a texture, and edit the processing expression.
   Confirm live compilation, alpha, scrolling at canvas zoom, and Code/AI tabs.
2. Add/change/reorder/remove metadata parameters. Check that unchanged ports
   preserve their values and cables, while removed/retyped ports disconnect.
3. Introduce a GLSL error and then a metadata error. Output and existing cables
   must stay active. Save Shader must show a modal and leave the disk file intact.
4. Generate and fix shaders through Requesty; check model persistence in presets.
5. Import an effect containing a relative include, save under a new name, restart,
   and compare the discovered Effects node with the live editor's output.
6. Save/recall a preset with an invalid draft, then with Live Compile off and an
   uncompiled valid draft. The previous working ports and shader must survive.
7. Load a feedback shader, connect a trigger to Clear, and save/recall the preset.
   Clear must exist before connections restore; failed edits must retain history.

## Shader save dialog and publisher

The default regression runner now also creates temporary local Git repositories
and verifies descriptions, category paths, symlink/traversal rejection, README
merging, the exact files in the publishing commit, author metadata, new folders,
repeat saves, and rejected pushes. It does not contact the real GitHub repository.

Host checks:

1. Choose an existing nested category, then a new one. Confirm the `.glsl`, category
   README, and library README are saved under data/Effects.
   The selector is labeled Category, with a [+] button after the label and a
   Create new category tooltip. Create selects a new category; Cancel leaves
   the selection unchanged. Invalid paths remain in the category dialog with an
   error. Confirm the old category text field and the two explanatory paragraphs
   are absent, with extra padding between dialog contents and window edges.
2. Generate an AI description and confirm the parameter header remains line 1,
   the old description is replaced, and both README catalogs use the new text.
   With an invalid API key, no local files should be written.
3. Edit source while description generation is pending. The file must contain the
   submitted snapshot while the editor retains the newer changes.
4. Select Push to GitHub with a configured SSH key. Confirm the target category,
   author metadata, shader, and merged README files in the same commit.
5. Retry an unchanged save with AI description disabled. No extra commit should
   be created. Publishing failures should preserve the local shader and report
   whether a recovery checkout was retained.
6. Edit both local and remote README documentation, then publish. Both copies
   must contain both sets of notes and one entry per shader in the catalog.

## Shader Inspector presentation

- Confirm there is no shader-source field in the Inspector; source still survives
  preset save/recall and remains editable in the Code tab.
- Confirm Shader Name, Shader Category, and Git SSH Key are labeled editable
  fields that apply changes on Enter.
- Repository, branch, repository folder, author name/email and Shader Valid must
  be informational text. Older presets must not override fixed Git settings.
- Save Status and Shader Status must wrap in secondary-colored text with no input
  background. Short messages reserve two lines; longer errors expand without
  clipping and remain visible at narrow Inspector widths.

- Saving messages appear immediately below Save Shader without a Save Status
  label. Git repository, branch and author values have no labels; the compact
  folder shows /SimpleEffectsLibrary/EffectsLibrary/... with its full category
  and shader path available in the tooltip.
- Separators divide saving, Git information, editor appearance and rendering
  controls. Hovering the compact folder shows the full shader destination.
