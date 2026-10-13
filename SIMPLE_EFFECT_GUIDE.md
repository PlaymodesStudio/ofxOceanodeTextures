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
- `Name:bool` creates a boolean checkbox, defaulting to false, bound to
  `uniform bool Name`. `Name:bool:true` or `Name:bool:1` defaults to true;
  `false` and `0` explicitly default to false. Booleans are scalar controls
  and do not accept control textures.
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

- The output is an RGBA32F texture matching `tSource` dimensions by default.
- A `// @swap-dimensions Name` annotation names a boolean metadata parameter.
  When true, the output width and height are exchanged. `uResolution`,
  `resolution`, and `uTexelSize` describe the output canvas; use
  `textureSize(tSource, 0)` for the source dimensions. Such canvas transforms
  render with blending disabled to preserve the shader's RGBA exactly.
  `Flip&Rotate` uses this contract for its `Rotate90` checkbox.
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

## Live Shader editor

Add **Textures → Shader**, connect an **Input** texture, and edit the complete
fragment source in its **Code** tab. The initial shader demonstrates the metadata
header, GLSL 410, standard uniforms, float/control-texture, color, and explicit
texture parameters. The input supplies the output dimensions, as in Simple
Effect; a procedural shader still needs an Input texture to define its canvas.

**Live Compile** is enabled by default and compiles after 400 ms without edits.
Use **Compile** or the inspector's **Reload Shader** for an immediate attempt.
A failed edit preserves the last working shader, controls, connections, and
history. Details appear in **Shader Status** and the editor status tooltip.
**Shader Valid** describes the running shader; it can remain true when the new
draft has errors. **Draw On Event** should stay off for continuous animation.

The Inspector hides the shader-source row; code remains available in the node's
Code tab and is still saved in presets. Shader Name, Shader Category, and Git SSH
Key have explicit labels and editable text fields (press Enter to apply changes).
Saving messages appear immediately below Save Shader without a status label.
Save Status and Shader Status are read-only, secondary-colored text without a
field background. Their message area reserves at least two lines and expands
for longer messages. Shader Valid is also read-only.

The **AI** tab opens the same Requesty assistant as Formula, including its API
key setup, model picker, Generate and Fix current modes. Its instructions include
the full shader contract; generated source undergoes the same validation and
compilation as manual edits. The selected AI model is stored in the preset.

Controls are created from line 1 after successful compilation. Existing controls
keep their values and connections when their name and type are unchanged;
changing a default in the header affects newly created controls. Range edits
update the limits. Renaming, removing, or changing a parameter's type removes its
old connections. Presets store both the draft and last working source, so an
unfinished edit can be reopened while retaining the working ports. With Live
Compile off, an uncompiled draft stays uncompiled on recall.

In the inspector, **Load Shader** imports a GLSL file and expands relative
`#include`/`#pragma include` files into the editor. **Save Shader** opens a dialog
with the shader name and a **Category** selector. The **[+]** button beside Category
opens a name dialog, with a **Create new category** hover hint. New categories
are selected immediately and can include nested folders; their directories are
created when the shader is saved. The dialogs have extra padding around their
contents. Files are saved below `bin/data/Effects/<category>/<name>.glsl`; the
category is created if needed. The last category is retained in presets. Saving
the same name overwrites that file. Names are restricted to letters, digits,
`_`, and `-` (spaces become `_`). New files appear as regular Effects nodes after
an app restart; existing effect instances can use Reload Shader.

**Generate AI description** is enabled by default in the save dialog. It uses the
same Requesty credentials and selected model as the AI tab to describe the actual
shader and its controls. The returned paragraph replaces existing
`// @description` annotations immediately after the metadata header. A failed
AI request saves nothing; retry or uncheck the option to retain the current
description. Generation runs in the background and saves a snapshot, so newer
editor changes are not overwritten by a completed request.

Each save updates `README.md` both in the chosen category and at the Effects
library root. Existing documentation is retained, with shader links and
`@description` text listed in an `oceanode-shaders` managed catalog block.
Repeating a save updates the entry rather than adding duplicates.

### Updating the Effects library from GitHub

**Config > Update Effects** downloads the public library from branch
`megabranch26_A`, folder `SimpleEffectsLibrary/EffectsLibrary`, and compares it
with the application's `data/Effects`. Git must be installed; downloading does
not require a GitHub account or the bot's SSH key. The download runs in the
background using an isolated sparse checkout. Other addon files and commits
are ignored, and no existing source checkout is changed.

Identical effects are left alone. Shared shader includes are expanded before
comparison so the original library layout is recognized; imported shaders are
self-contained and do not replace local shared includes. Invalid shaders and
unsafe paths are skipped with details in the result. Local-only effects and
files removed from the repository are retained.

If there are differences, a review dialog lists each category/file. Expand an
entry to see the changed range, with `-` for local lines and `+` for repository
lines. Choose **Keep local**, **Use repository**, or **Back up local + use
repository** for each file, then click **Apply selected changes**. Keep local is
the default. Cancel imports nothing. When there are no conflicts, new shaders
are imported immediately and a result is shown. The result lists each successfully
added shader with its category/file path under **New effects**. This list remains
visible until the dialog is closed and resets when starting another update.

Backups are written under `data/EffectsBackups/Update-...`, preserving category
paths and the replaced shader's local include dependencies. A file or include
edited while the review is open is kept and reported for a fresh review.
Category and library README catalogs are merged with existing documentation and
describe the actual local files retained. New effects appear in New Node without
restarting; existing effect instances use **Reload Shader** to load a replaced
file. Their running shaders and controls are not changed automatically.

### Publishing shader saves to GitHub

**Push to GitHub** is off by default and must be selected for each save. Inspector
settings initially target `https://github.com/PlaymodesStudio/ofxOceanodeTextures.git`,
branch `megabranch26_A`, library folder `SimpleEffectsLibrary/EffectsLibrary`. The relative
category path is preserved below that repository folder. These three settings
are displayed as compact read-only information without repository/branch labels.
The folder line shows `/SimpleEffectsLibrary/EffectsLibrary/...`; hover it for
the full shader path, including its category. Author name/email also appear
without labels. Separators divide saving, Git publishing, editor appearance and
rendering controls. The repository, branch, library folder, author name, and
author email are fixed; older preset values cannot
override them. Only the local SSH-key path is editable.

Publishing uses an isolated temporary checkout of the target branch and creates
missing category folders. It merges the local and repository README documentation
and catalog entries, commits the shader and affected READMEs together, then pushes
normally. After a successful push, the same merged READMEs are written locally.
Other files and existing local repository checkouts are untouched. If the branch
moves while publishing, the normal push is rejected; retry the save to start
from the new remote state. A failed publish retains the successful local save,
reports the error in **Save Status**, and retains the temporary checkout when
there is a recoverable commit.

Git must be installed, and the pushing identity must have Write access to the
repository and permission to push to the selected branch. **Git SSH Key** accepts
an absolute private-key path for a dedicated automation account. A new Shader
node automatically selects `~/.ssh/ofxOceanode_shader_ed25519` when that file
exists on this computer. When set with a
GitHub HTTPS URL, the publisher uses SSH with that specific key. The key must be
usable non-interactively (for encrypted keys, unlock it in the SSH agent first).
With no dedicated key selected, Git uses the computer's existing authentication
configuration. Secrets are never stored in the shader, presets, or repository;
only the key path is saved. Do not put tokens/passwords in the repository URL.

**Git Author Name** and **Git Author Email** initially use `infoplaymodes`
and `infoplaymodes@gmail.com`; these are commit metadata, separate from the login
that authenticates the push. To use a new GitHub machine-user account, complete
its signup/email verification, add the SSH public key to that account, and grant
it Write access to this repository. A public repository still requires permission
for pushing. The author name and email are fixed in this editor.

Saving requires valid first-line metadata (`//` is valid for no controls),
`#version 410` or `#version 410 core` as the first non-comment directive, matching
parameter uniform declarations, a `vec4` fragment output, and successful GPU
compilation/linking. Names must not collide with built-in ports, inspector/preset
fields, standard uniforms, or generated control-texture uniforms. Active uniforms
must have types the host can supply. Include directives must be expanded, making
the saved file independent of its original directory. A native modal lists any
deficiencies and **no file is written** when validation fails.
