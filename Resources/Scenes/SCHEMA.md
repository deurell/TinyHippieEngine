# Scene Description Schema

Scene descriptions are JSON files used for text-authored composition. They
create normal runtime nodes; gameplay behavior should still bind typed C++
objects by name.

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

## Node Types

- `SceneNode`: hierarchy/transform only.
- `CameraNode`: uses `active`, `fov`, and optional `lookAt`.
- `LightNode`: uses `light` to provide one scene directional light for lit
  renderers (`MeshNode` and `PhongShapeNode`).
- `MeshNode`: uses `mesh`, optional `animation`, optional `visualizer`.
- `SpriteNode`: uses `image`; optional `sourceRect` selects an atlas region in
  source pixels as `[x, y, width, height]`; optional `flipX`, `flipY`, and
  `flipDiagonal` mirror/swap the selected region for Tiled-style atlas
  transforms; optional `billboard` makes it face the scene camera each frame.
- `SpriteBatchNode`: uses `spriteBatch` to render many static atlas sprites
  from one image as one mesh/draw command.
- `TextNode`: uses `text`; optional `alignment` (`Left`, `Center`, `Right`),
  optional `anchor` (`TopLeft`, `TopCenter`, `TopRight`, `CenterLeft`,
  `Center`, `CenterRight`, `BottomLeft`, `BottomCenter`, `BottomRight`), and
  optional `fontSize`, `textColor`, `shadowColor`, `shadowOffset`, and
  `billboard` makes it face the scene camera each frame.
- `TileMapNode`: uses `tileMap` to render an atlas-backed orthogonal tile map
  as one static mesh. Layer `data` values use Tiled global tile IDs, including
  horizontal, vertical, and diagonal flip flags.
- `PlaneNode`: uses `plane` (`Simple` or `Spinner`) and optional `color`.
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
  "fov": 40.0,
  "lookAt": [0.0, 0.75, 0.15]
}
```

The active root camera is built first by `TextStarterScene` so other nodes can
render through it.

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
lit renderers only: `MeshNode` and `PhongShapeNode`. Sprites, sprite batches,
tile maps, text, particles, overlays, and postprocess are unlit.

Omit `direction` to derive the light direction from the node rotation.

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
  "visualizer": {
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

`plane` may be `Simple` or `Spinner`.

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
