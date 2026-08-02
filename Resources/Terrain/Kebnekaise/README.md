# Kebnekaise terrain sample

This scene covers an approximately 11 km square around Kebnekaise, Sweden.
Horizontal and vertical geometry use real metres at a display scale of one
engine unit per 200 metres. The orange route visualizes OpenStreetMap relation
10528584, Västra leden. It is a visualization, not a navigational product.

Elevation was derived from the public Mapzen/Terrain Tiles Terrarium dataset at
zoom 12. Terrain Tiles are provided under CC BY 4.0; elevation sources and
attribution are documented by the Tilezen project:
https://github.com/tilezen/joerd/blob/master/docs/data-sources.md

Regenerate the checked-in indexed terrain and 3D route glTF assets with:

```bash
scripts/generate_kebnekaise_terrain.py TILE_DIRECTORY Resources/Terrain/Kebnekaise
```

The expected source tile range is x `2257..2259`, y `982..984`, zoom `12`.
The script also emits a Gazebo-compatible source package under `GazeboSource/`
and runs that package through the generic importer. The live scene loads the
checked-in `kebnekaise_imported_*` terrain and route assets. The separate
Joshimath dataset remains available as an importer reference fixture.
Route geometry is OpenStreetMap data, available under ODbL:
https://www.openstreetmap.org/copyright

## Importing gazebo_terrain_generator output

Terrain produced by `gazebo_terrain_generator` can be converted directly from
its generated folder (the folder containing the `.world`/`.sdf` and `mesh/`):

```bash
scripts/import_gazebo_terrain.py /path/to/generated/model Resources/Terrain/MyArea \
  --name my_area --route /path/to/route.gpx
```

The route is optional and may be GPX, GeoJSON, or Overpass JSON. The importer
reads physical dimensions, terrain position, geographic origin, heightmap, and
aerial texture from the generated SDF. It writes indexed glTF terrain, an
optional raised 3D route mesh, and a `.terrain.json` manifest. It does not need
Gazebo or a map-provider token; those are only needed while generating the
source world.
