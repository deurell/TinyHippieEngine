# Deflektorish Levels

Levels are plain JSON game data. They describe mechanics and placement only;
the C++ scene owns runtime state, node creation, shader styles, and input.

Coordinates use grid cells. The default grid maps a cell to game pixels with:

```text
position = cell * grid.tileSize + grid.origin
```

Angles are authored in degrees. Automatic `reflektors` and `filters` rotate by
`speed` radians per second at runtime.

Completed playtest times are appended to
`.tiny_hippie_storage/deflektorish_level_times.json` on desktop. Web builds
store the same JSON under the `deflektorish_level_times.json` local-storage
key. Each record includes the one-based level number, authored level name,
clear time in seconds, Unix timestamp, and a `debugSkip` flag. Completions
triggered with the `V` debug shortcut are flagged so they can be excluded from
balancing data.

For rapid crawler iteration, press `F4` through `F10` in the game scene to
rebuild the campaign runtime and jump directly to the matching level. Pressing
the same key again fully restarts that level, including enemy spawn timers and
latch state. These shortcuts, `V` victory skip, and `B` background toggle are
compiled out with
`-DTINY_ENGINE_ENABLE_DEFLEKTORISH_DEBUG_KEYS=OFF`.

Top-level fields:

- `name`: level id for humans and logs.
- `parTimeSeconds`: clear-time target used by the time bonus. This is calibrated
  independently per level and can be revised as playtest data accumulates.
- `grid`: `{ "tileSize": number, "origin": [x, y] }`.
- `source`: `{ "cell": [x, y], "angleDegrees": number }`.
- `explosionPoolSize`: number of reusable explosion visuals.
- `reflektors`: mirrors; manual when `automatic` is false.
- `targets`: objects destroyed by the beam.
- `blockers`: solid or reflective blockers.
- `portals`: beam teleport pairs.
- `filters`: angle filters; automatic rotation is optional.
- `splitters`: beam splitters.
- `enemies`: standalone crawler nests. Each entry authors `spawnCell`, a
  zero-based preferred `targetReflektorIndex`, initial `spawnDelay`, repeating
  `spawnInterval`, movement `speed` in pixels per second, and sustained beam
  contact `destroySeconds`. `maxSpawns` caps each finite wave. Nests repeat
  subject to the global three-crawler cap and distribute crawlers across
  available manual reflektors. Holding the beam on a nest destroys it and
  stops future spawns. Nest and crawler kills restore beam energy; reaching a
  reflektor locks it until the crawler is hit.

Example:

```json
{
  "cell": [12, 7],
  "angleDegrees": 45,
  "automatic": false
}
```
