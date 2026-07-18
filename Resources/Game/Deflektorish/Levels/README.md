# Deflektorish Levels

Levels are plain JSON game data. They describe mechanics and placement only;
the C++ scene owns runtime state, node creation, shader styles, and input.

Coordinates use grid cells. The default grid maps a cell to game pixels with:

```text
position = cell * grid.tileSize + grid.origin
```

Angles are authored in degrees. Automatic `reflektors` and `filters` rotate by
`speed` radians per second at runtime.

Top-level fields:

- `name`: level id for humans and logs.
- `grid`: `{ "tileSize": number, "origin": [x, y] }`.
- `source`: `{ "cell": [x, y], "angleDegrees": number }`.
- `explosionPoolSize`: number of reusable explosion visuals.
- `reflektors`: mirrors; manual when `automatic` is false.
- `targets`: objects destroyed by the beam.
- `blockers`: solid or reflective blockers.
- `portals`: beam teleport pairs.
- `filters`: angle filters; automatic rotation is optional.
- `splitters`: beam splitters.

Example:

```json
{
  "cell": [12, 7],
  "angleDegrees": 45,
  "automatic": false
}
```
