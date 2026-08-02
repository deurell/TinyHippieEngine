#!/usr/bin/env python3
"""Local COG conversion test; does not contact Lantmäteriet."""

import argparse
import tempfile
from pathlib import Path

import numpy as np
import rasterio
from pyproj import Transformer
from rasterio.transform import from_bounds

import generate_lantmateriet_terrain as generator
import import_gazebo_terrain


def main() -> int:
    bbox = (18.50, 67.89, 18.52, 67.91)
    transformer = Transformer.from_crs("EPSG:4326", generator.TARGET_CRS, always_xy=True)
    west, south = transformer.transform(bbox[0], bbox[1])
    east, north = transformer.transform(bbox[2], bbox[3])
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        elevation_path = root / "elevation.tif"
        aerial_path = root / "aerial.tif"
        transform = from_bounds(west, south, east, north, 8, 8)
        profile = {"driver": "GTiff", "width": 8, "height": 8, "crs": generator.TARGET_CRS,
                   "transform": transform, "dtype": "float32", "count": 1}
        with rasterio.open(elevation_path, "w", **profile) as dataset:
            dataset.write(np.linspace(500, 1500, 64, dtype=np.float32).reshape(8, 8), 1)
        profile.update(dtype="uint8", count=3)
        with rasterio.open(aerial_path, "w", **profile) as dataset:
            pixels = np.empty((3, 8, 8), dtype=np.uint8)
            pixels[0], pixels[1], pixels[2] = 80, 120, 60
            dataset.write(pixels)
        elevation_asset = generator.Asset("height", str(elevation_path), generator.TARGET_CRS, "2026", "fixture")
        image_asset = generator.Asset("image", str(aerial_path), generator.TARGET_CRS, "2026", "fixture")
        elevation, _ = generator.mosaic([elevation_asset], bbox, 9, 9, 1, None, None)
        aerial, _ = generator.mosaic([image_asset], bbox, 16, 16, 3, None, None)
        source, mesh, output = root / "source", root / "source" / "mesh", root / "output"
        mesh.mkdir(parents=True)
        output.mkdir()
        minimum, height_range = generator.save_heightmap(elevation[0], mesh / "height_map.png")
        generator.save_aerial(aerial, mesh / "aerial.png")
        generator.save_normal_map(elevation[0], east - west, north - south, mesh / "normal_map.png")
        world = generator.write_world(source, "fixture", bbox, east - west, north - south,
                                      minimum, height_range)
        spec = import_gazebo_terrain.read_spec(world)
        import_gazebo_terrain.write_terrain(spec, output, "fixture", 9, 200, 16)
        assert (output / "fixture_terrain.gltf").is_file()
        assert 499 < minimum < 501
        assert 999 < height_range < 1001
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
