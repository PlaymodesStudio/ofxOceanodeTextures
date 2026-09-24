# ofxOceanodeTextures

Texture processing nodes and a [catalog of 93 Simple Effects shaders](SimpleEffectsLibrary/EffectsLibrary/README.md).

- [Shader authoring, input history, and output feedback](SIMPLE_EFFECT_GUIDE.md)
- [Optical Flow: motion vectors, masks, confidence, and previews](SimpleEffectsLibrary/EffectsLibrary/README.md#motion-analysis)
- [Feedback: camera patch, example settings, and manual tests](SimpleEffectsLibrary/EffectsLibrary/README.md#temporal)
- [Texture Feedback Delay: patch any effect into the loop](SimpleEffectsLibrary/EffectsLibrary/README.md#patchable-feedback-loop)
- [Presence Detector: camera occupancy, lightness, and setup](PRESENCE_DETECTOR_GUIDE.md)

Copy the contents of `SimpleEffectsLibrary/EffectsLibrary/` into the application's
`bin/data/Effects/`, preserving category folders and `SimpleEffectCommon.inc`.
Rebuild the application after C++ host changes, and restart it to discover newly
added shader files. Optical Flow requires the input-history support in
`src/simpleEffect.h`; Feedback requires its previous-output support.
`Texture Feedback Delay` and `Presence Detector` are C++ nodes; rebuild and
restart the application to register them.
