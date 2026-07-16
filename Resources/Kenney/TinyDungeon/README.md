# Kenney Tiny Dungeon

Source assets from Kenney Tiny Dungeon, CC0.

The starter scene is generated from the Tiled sample map:

```bash
scripts/convert_tiled_map.py \
  Resources/Kenney/TinyDungeon/Tiled/sampleMap.tmx \
  Resources/Scenes/tiny_dungeon_atlas.scene.json \
  --repo-root . \
  --scene-name kenney_tiny_dungeon_atlas_sample \
  --node-name tiny_dungeon_map \
  --tile-world-size 0.34 \
  --use-packed \
  --camera-position 0.0,0.0,10.5 \
  --camera-look-at 0.0,0.0,0.0 \
  --camera-fov 38.0 \
  --hero-marker hero \
  --layer-z Dungeon=0.0 \
  --layer-z Objects=0.035 \
  --layer-z Carts=0.07
```
