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
- `MeshNode`: uses `mesh`, optional `animation`, optional `visualizer`.
- `SpriteNode`: uses `image`; optional `billboard` makes it face the scene
  camera each frame.
- `TextNode`: uses `text`; optional `alignment` (`Left`, `Center`, `Right`),
  optional `anchor` (`TopLeft`, `TopCenter`, `TopRight`, `CenterLeft`,
  `Center`, `CenterRight`, `BottomLeft`, `BottomCenter`, `BottomRight`), and
  optional `fontSize`, `textColor`, `shadowColor`, `shadowOffset`, and
  `billboard` makes it face the scene camera each frame.
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

## PhongShapeNode Material

```json
{
  "material": {
    "diffuse": [0.8, 0.8, 0.8],
    "ambient": [0.25, 0.25, 0.25],
    "specular": [0.2, 0.2, 0.2],
    "shininess": 16.0
  }
}
```
