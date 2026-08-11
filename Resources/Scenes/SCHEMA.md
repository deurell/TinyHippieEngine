# Scene Description Schema

Scene descriptions are JSON files used for text-authored composition. They
create normal runtime nodes; gameplay behavior should still bind typed C++
objects by name.

Validate authored scenes with:

```bash
scripts/tiny_hippie_validate.py Resources/Scenes/*.scene.json
```

The validator checks JSON shape, known fields, node types, required node
payloads, enum values, tilemap/atlas invariants, and referenced asset paths. It
may warn about suspicious transforms; warnings are advisory unless tooling
explicitly promotes them.

## Root

```json
{
  "name": "scene_name",
  "nodes": []
}
```

## Common Node Fields

Every node supports:

```json
{
  "name": "node_name",
  "type": "SceneNode",
  "transform": {
    "position": [0.0, 0.0, 0.0],
    "rotationEuler": [0.0, 0.0, 0.0],
    "scale": [1.0, 1.0, 1.0]
  },
  "children": []
}
```

`rotationEuler` is in degrees. Scene units are meters: use `1.0` as roughly one
meter for authored positions, scales, camera movement, and gameplay-facing
dimensions.

Visual nodes may set `renderLayer` as an integer. Lower layers draw first;
higher layers draw later. Within one layer, opaque/depth-tested items draw
before transparent items, and transparent items with `BackToFront` sorting draw
far-to-near.

## Node Types

- `SceneNode`: hierarchy/transform only.
- `CameraNode`: uses `active`, optional `projection` (`Perspective` or
  `Orthographic`), `fov`, `orthographicHeight`, and optional `lookAt`.
- `LightNode`: uses `light` to provide one scene directional light for lit
  render components (`MeshNode` and `PhongShapeNode`).
- `Light2DNode`: uses `light2D` to draw an unlit additive radial glow for 2D
  scenes.
- `MeshNode`: uses `mesh`, optional `animation`, optional `renderSettings`.
- `SpriteNode`: uses `image`; optional `sourceRect` selects an atlas region in
  source pixels as `[x, y, width, height]`; optional `flipX`, `flipY`, and
  `flipDiagonal` mirror/swap the selected region for Tiled-style atlas
  transforms; optional `billboard` makes it face the scene camera each frame.
- `SpriteAnimationNode`: uses `image` and `spriteAnimation` to play atlas-frame
  animation by changing the active source rectangle during `fixedUpdate()`.
- `SpriteBatchNode`: uses `spriteBatch` to render many static atlas sprites
  from one image as one mesh/draw command.
- `FogOverlayNode`: uses `fogOverlay` to render a transparent scrolling texture
  overlay for mist, clouds, or cloud shadows.
- `TextNode`: uses `text`; optional `alignment` (`Left`, `Center`, `Right`),
  optional `anchor` (`TopLeft`, `TopCenter`, `TopRight`, `CenterLeft`,
  `Center`, `CenterRight`, `BottomLeft`, `BottomCenter`, `BottomRight`), and
  optional `fontSize`, `textColor`, `shadowColor`, `shadowOffset`, and
  `billboard` makes it face the scene camera each frame.
- `TileMapNode`: uses `tileMap` to render an atlas-backed orthogonal tile map
  as one static mesh. Layer `data` values use Tiled global tile IDs, including
  horizontal, vertical, and diagonal flip flags.
- `PlaneNode`: uses `plane` (`Simple` or `Spinner`) and optional `color`.
- `ShaderPlaneNode`: uses `shaderPlane` to render a quad with authored shader
  paths, blend mode, depth test, and two vec4 parameter buckets.
- `PhongShapeNode`: uses `shape` (`Cube`, `Sphere`, or `Cylinder`) and optional
  `material`.
- `ParticleSystemNode`: uses `particle` (`Default`, `SoftGlowBurst`, or
  `WaterFountain`); optional `billboard` makes particle quads face the scene
  camera.

## CameraNode

```json
{
  "type": "CameraNode",
  "active": true,
  "projection": "Perspective",
  "fov": 40.0,
  "orthographicHeight": 6.0,
  "lookAt": [0.0, 0.75, 0.15]
}
```

The active root camera is built first by `TextStarterScene` so other nodes can
render through it.
`fov` is used by perspective cameras. `orthographicHeight` is the vertical
world-space height visible through orthographic cameras; width is derived from
the framebuffer aspect ratio.

## LightNode

```json
{
  "type": "LightNode",
  "light": {
    "kind": "Directional",
    "direction": [0.35, 1.0, 0.25],
    "color": [1.0, 0.94, 0.82],
    "intensity": 1.0,
    "ambientStrength": 0.42,
    "active": true
  }
}
```

`LightNode` currently supports one simple forward directional light. It affects
lit render components only: `MeshNode` and `PhongShapeNode`. Sprites, sprite batches,
tile maps, text, particles, overlays, and postprocess are unlit.

Omit `direction` to derive the light direction from the node rotation.

## Light2DNode

```json
{
  "type": "Light2DNode",
  "light2D": {
    "color": [1.0, 0.46, 0.14, 1.0],
    "radius": 1.25,
    "intensity": 0.42,
    "softness": 0.82,
    "flickerAmount": 0.16,
    "flickerSpeed": 7.5
  }
}
```

`Light2DNode` renders an additive radial overlay. It is intentionally unlit and
does not affect sprite, tilemap, mesh, or text shading. Use it for authored
glows such as torches, magic, and pickups in 2D atlas scenes.

## MeshNode

```json
{
  "type": "MeshNode",
  "mesh": "Resources/character-l.glb",
  "animation": {
    "clip": "idle",
    "playing": true,
    "looping": true,
    "playbackSpeed": 1.0
  },
  "renderSettings": {
    "lightDirection": [0.35, 1.0, 0.25],
    "lightColor": [1.0, 0.96, 0.9],
    "ambientStrength": 0.42,
    "specularStrength": 0.12,
    "shininess": 24.0
  }
}
```

## SpriteNode Atlas Region

`SpriteNode` renders one quad. Use it for individual sprites, props, markers,
and occasional atlas regions. Use `TileMapNode` for dense tile maps.

```json
{
  "type": "SpriteNode",
  "image": "Resources/Kenney/TinyDungeon/Tilemap/tilemap_packed.png",
  "sourceRect": [32.0, 48.0, 16.0, 16.0],
  "flipX": false,
  "flipY": false,
  "flipDiagonal": false
}
```

`sourceRect` uses top-left image coordinates in pixels. Omit it to render the
full image.

## SpriteAnimationNode

`SpriteAnimationNode` renders one quad like `SpriteNode`, but advances through
atlas frame rectangles during `fixedUpdate()`. Use it for simple retro
characters, torches, pickups, signs, and other hand-authored frame animation.

```json
{
  "type": "SpriteAnimationNode",
  "image": "Resources/Kenney/TinyDungeon/Tilemap/tilemap_packed.png",
  "billboard": true,
  "spriteAnimation": {
    "fps": 8.0,
    "playing": true,
    "looping": true,
    "frames": [
      [0.0, 32.0, 16.0, 16.0],
      [16.0, 32.0, 16.0, 16.0],
      [32.0, 32.0, 16.0, 16.0]
    ]
  }
}
```

Each frame is `[x, y, width, height]` in top-left image pixel coordinates.

## SpriteBatchNode

`SpriteBatchNode` renders many static quads from one image. Use it for dense
retro props, atlas decoration, pickups, foliage, signs, and other static sprite
sets that should batch together. Use `SpriteNode` for one-off independently
controlled sprites.

```json
{
  "type": "SpriteBatchNode",
  "spriteBatch": {
    "image": "Resources/Kenney/TinyDungeon/Tilemap/tilemap_packed.png",
    "sprites": [
      {
        "position": [0.0, 0.0, 0.0],
        "size": [0.5, 0.5],
        "rotationDegrees": 0.0,
        "sourceRect": [32.0, 48.0, 16.0, 16.0],
        "flipX": false,
        "flipY": false,
        "flipDiagonal": false
      }
    ]
  }
}
```

Each sprite `position` is local to the batch node. `size` is in scene units.
`sourceRect` uses top-left image coordinates in pixels. Omit `sourceRect` to
use the full image for that sprite.

## FogOverlayNode

```json
{
  "type": "FogOverlayNode",
  "fogOverlay": {
    "image": "Resources/Textures/generated/fog-soft-noise.png",
    "color": [0.66, 0.82, 0.9, 1.0],
    "tiling": [2.6, 1.7],
    "scrollSpeed": [0.012, 0.004],
    "alpha": 0.14,
    "softness": 0.72,
    "secondLayerStrength": 0.48,
    "secondLayerScrollSpeed": [-0.006, 0.009],
    "pulseAmount": 0.035,
    "pulseSpeed": 0.45
  },
  "transform": {
    "position": [0.0, 0.0, 0.95],
    "scale": [6.8, 4.2, 1.0]
  }
}
```

`FogOverlayNode` renders a textured quad with alpha blending and depth disabled.
Use `scale` for world-space coverage, `tiling` for texture repeat density,
`scrollSpeed` for drift, and `alpha` as the main visible opacity control.
`secondLayerStrength` blends a second scrolling sample from the same texture to
avoid a single obvious sliding pattern.

## TileMapNode

```json
{
  "type": "TileMapNode",
  "tileMap": {
    "image": "Resources/Kenney/TinyDungeon/Tilemap/tilemap_packed.png",
    "firstGid": 1,
    "mapWidth": 32,
    "mapHeight": 20,
    "tileWidth": 16,
    "tileHeight": 16,
    "columns": 12,
    "tileWorldSize": 0.34,
    "layers": [
      {
        "name": "Dungeon",
        "z": 0.0,
        "data": [1, 2, 0, 0]
      }
    ]
  }
}
```

`data` is row-major and may include Tiled flip bits. `0` means no tile.
`firstGid` is the first Tiled global tile ID for the tileset used by the map.

`TileMapNode` deliberately supports a simple regular-grid atlas, not an
arbitrary rectangle-packed sprite atlas:

- Every atlas tile has the same pixel dimensions (`tileWidth` by
  `tileHeight`).
- Tiles are laid out from left to right in rows, with `columns` tiles per row.
- The atlas has no margin, spacing, or rotated entries.
- One map uses one tileset and one atlas image.
- `tileWorldSize` is a single scalar, so rendered cells are square in world
  space even if the source pixels are rectangular.
- Tiled tile-animation metadata is not imported. Runtime code may replace
  cells, but the node does not advance authored animation sequences itself.

The atlas tile index is derived from the Tiled global ID after removing Tiled's
horizontal, vertical, and diagonal flip bits and subtracting `firstGid`. Large
values such as `1610612787` therefore represent an ordinary tile ID plus flip
flags; they are not intended to be interpreted manually.

The first `data` value is the top-left map cell. Rows proceed downward in the
JSON data. At runtime the complete map is centered on the `TileMapNode` origin,
with positive world Y pointing upward. Use the node `transform` to position,
rotate, or scale the complete map, and use each layer's `z` to control layer
ordering.

Runtime code can query, replace, or clear a cell without editing the authored
scene description:

```cpp
if (tileMap.setTile("Dungeon", {2, 2}, {.tileIndex = 10})) {
  // Cell (2, 2) now displays atlas tile 10.
}

tileMap.clearTile("Dungeon", {2, 2});
const auto cell = tileMap.tile("Dungeon", {2, 2}); // empty after clear
const glm::vec2 center = tileMap.mapToLocal({2, 2});
```

Runtime `tileIndex` values are zero-based atlas indices. This differs from JSON
`data`, which stores Tiled global IDs: `0` means empty and the first tile is
`firstGid`. `setTile` and `clearTile` return `false` for an unknown layer or an
out-of-bounds coordinate; `clearTile` also returns `false` when the cell was
already empty. `tile` returns `std::nullopt` for both an empty cell and an
invalid lookup.

Multiple cell edits are cheap to issue together: they only mark the tile map
dirty, and `TileMapNode` rebuilds its single batched mesh once during the next
`update()`. Make gameplay-driven changes in `fixedUpdate()`; rendering catches
them on the following presentation update. For a small number of animated
decorations, leave those cells empty in Tiled and place `SpriteAnimationNode`
children at positions returned by `mapToLocal`. Native animated map cells would
require importing each Tiled frame sequence and changing cells from
`fixedUpdate()`.

### Tiled conversion workflow

Author the tile grid visually in Tiled and save an orthogonal `.tmx` map with
exactly one tileset and CSV-encoded tile layers. Convert it from the repository
root:

```bash
scripts/convert_tiled_map.py \
  path/to/map.tmx \
  Resources/Scenes/my_map.scene.json \
  --tile-world-size 0.34 \
  --layer-step 0.01 \
  --use-packed
```

`--use-packed` selects a sibling image named `*_packed.png` when one exists;
this image must still be a regular fixed-cell grid. Use repeated
`--layer-z NAME=VALUE` options when particular Tiled layers need explicit
depths. Run `scripts/convert_tiled_map.py --help` for camera, naming, and marker
options.

The converter writes a complete scene containing a camera, an optional marker,
and the `TileMapNode`. It replaces the output file. Hand-authored nodes added to
that generated file will be lost on reconversion, so keep the `.tmx`/`.tsx`
files as the visual map source and reconvert deliberately. Validate the result
with:

```bash
scripts/tiny_hippie_validate.py Resources/Scenes/my_map.scene.json
```

## TextNode

```json
{
  "type": "TextNode",
  "text": "Sample",
  "alignment": "Center",
  "anchor": "BottomCenter",
  "fontSize": 42.0,
  "textColor": [1.0, 1.0, 1.0, 1.0],
  "shadowColor": [0.0, 0.0, 0.0, 0.58],
  "shadowOffset": [1.5, -1.5],
  "billboard": true
}
```

## PlaneNode

```json
{
  "type": "PlaneNode",
  "plane": "Simple",
  "color": [0.2, 0.45, 0.8, 1.0]
}
```

`plane` may be `Simple` or `Spinner`. Use `ShaderPlaneNode` for custom
shader-driven quads.

## ShaderPlaneNode

```json
{
  "type": "ShaderPlaneNode",
  "color": [1.0, 1.0, 1.0, 1.0],
  "shaderPlane": {
    "vertexShader": "Shaders/simple.vert",
    "fragmentShader": "Shaders/deflektorish.frag",
    "blendMode": "Additive",
    "depthTest": false,
    "proceduralStyle": 1,
    "params0": [0.0, 0.0, 0.0, 0.0],
    "params1": [0.0, 0.0, 0.0, 0.0]
  }
}
```

`blendMode` may be `Opaque`, `Alpha`, or `Additive`. Shaders receive standard
`iTime`, `baseColor`, `model`, `view`, and `projection` uniforms, plus
`proceduralStyle`, `proceduralParams`, and `proceduralParams2`.

## PhongShapeNode Material

```json
{
  "type": "PhongShapeNode",
  "shape": "Cube",
  "material": {
    "diffuse": [0.8, 0.8, 0.8],
    "ambient": [0.25, 0.25, 0.25],
    "specular": [0.2, 0.2, 0.2],
    "shininess": 16.0
  }
}
```

`shape` may be `Cube`, `Sphere`, or `Cylinder`.

## ParticleSystemNode

```json
{
  "type": "ParticleSystemNode",
  "particle": "WaterFountain",
  "billboard": false
}
```

`particle` may be `Default`, `SoftGlowBurst`, or `WaterFountain`.
