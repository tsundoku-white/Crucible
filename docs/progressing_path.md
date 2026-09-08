## Pre-Alpha v0.01

### Core / ECS
[x] resource manager
[x] ECS registry (entity/component)
[x] Transform, Camera, Model components
[ ] proper debug logging
[ ] event/messaging system (for decoupling input -> gameplay -> render)

### Rendering
[x] model loading
[x] dynamic same-mesh batching (instancing)
[ ] texture loading
[ ] dynamic texture recycling
[ ] material system (separate from mesh data)
[ ] lights
[ ] light maps (maybe)
[ ] frustum culling
[ ] skybox

### Input / Camera
[x] window + input polling
[x] free-fly camera (WASD + mouse look)
[ ] controller support
[ ] input action-mapping layer (decouple keys from actions)

### UI
[ ] lua ui manager
[ ] ui elements (button, image, slider, text, check-box, vbox, hbox)

### Audio
[ ] audio playback
[ ] 3D/positional audio

### Gameplay systems
[ ] collision detection
[ ] scene serialization (save/load level)

## v0.1.0
[ ] animation support
[ ] skeletal mesh + bone components (needed before animation)

## Versioning

Format: `vMAJOR.MINOR.PATCH` (e.g. `v0.14.312`)

- **MAJOR** — stage-defining change: massive rework or extension of the current framework (e.g. Pre-Alpha -> Alpha).
- **MINOR** — one mid-size update: a checklist item goes from not-working to working. Small fixes, refactors, and cleanup ride along silently and don't bump the version on their own.

- **PATCH** for minimal fixes that is not big but could be noted.
