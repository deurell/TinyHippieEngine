#!/usr/bin/env python3
"""Generate Tiny Hippie terrain from Lantmäteriet elevation and orthophoto COGs.

Requires rasterio, pyproj, numpy, and Pillow. Credentials are read from
LANTMATERIET_USERNAME / LANTMATERIET_PASSWORD and are never persisted.
"""

from __future__ import annotations

import argparse
import json
import os
import sys
import urllib.parse
import urllib.request
from dataclasses import dataclass
from pathlib import Path

import numpy as np
import rasterio
from PIL import Image
from pyproj import Transformer
from rasterio.enums import Resampling
from rasterio.transform import from_bounds
from rasterio.warp import reproject

import import_gazebo_terrain


HEIGHT_STAC = "https://api.lantmateriet.se/stac-hojd/v1"
IMAGE_STAC = "https://api.lantmateriet.se/stac-bild/v1"
TARGET_CRS = "EPSG:3006"


@dataclass(frozen=True)
class Asset:
    item_id: str
    href: str
    crs: str
    datetime: str
    collection: str


def request_json(url: str) -> dict:
    request = urllib.request.Request(url, headers={"User-Agent": "TinyHippieTerrain/1.0"})
    with urllib.request.urlopen(request) as response:
        return json.load(response)


def discover_assets(stac_url: str, bbox: tuple[float, float, float, float]) -> list[Asset]:
    query = urllib.parse.urlencode({"bbox": ",".join(map(str, bbox)), "limit": 100})
    url = f"{stac_url}/search?{query}"
    assets: dict[str, Asset] = {}
    while url:
        page = request_json(url)
        for feature in page.get("features", []):
            data = feature.get("assets", {}).get("data")
            if not data:
                continue
            assets[data["href"]] = Asset(
                item_id=feature["id"], href=data["href"],
                crs=data.get("proj:code") or feature.get("properties", {}).get("proj:code", TARGET_CRS),
                datetime=feature.get("properties", {}).get("datetime", ""),
                collection=feature.get("collection", ""))
        url = next((link["href"] for link in page.get("links", [])
                    if link.get("rel") == "next"), "")
    return sorted(assets.values(), key=lambda asset: (asset.datetime, asset.item_id))


def newest_collection(assets: list[Asset]) -> list[Asset]:
    """Keep the collection containing the newest matching acquisition."""
    if not assets:
        return []
    selected = max(assets, key=lambda asset: asset.datetime).collection
    return [asset for asset in assets if asset.collection == selected]


def gdal_environment(username: str | None, password: str | None) -> rasterio.Env:
    options = {"GDAL_HTTP_MULTIRANGE": "YES", "GDAL_DISABLE_READDIR_ON_OPEN": "EMPTY_DIR"}
    if username and password:
        options["GDAL_HTTP_USERPWD"] = f"{username}:{password}"
        options["GDAL_HTTP_AUTH"] = "BASIC"
    return rasterio.Env(**options)


def mosaic(assets: list[Asset], bbox: tuple[float, float, float, float],
           width: int, height: int, bands: int, username: str | None,
           password: str | None) -> tuple[np.ndarray, rasterio.Affine]:
    if not assets:
        raise RuntimeError("the STAC search returned no assets for this area")
    transformer = Transformer.from_crs("EPSG:4326", TARGET_CRS, always_xy=True)
    west, south = transformer.transform(bbox[0], bbox[1])
    east, north = transformer.transform(bbox[2], bbox[3])
    transform = from_bounds(west, south, east, north, width, height)
    destination = np.full((bands, height, width), np.nan, dtype=np.float32)
    with gdal_environment(username, password):
        for index, asset in enumerate(assets, 1):
            print(f"[{index}/{len(assets)}] {asset.item_id}", file=sys.stderr)
            try:
                with rasterio.open(asset.href) as source:
                    for band in range(min(bands, source.count)):
                        reproject(
                            source=rasterio.band(source, band + 1),
                            destination=destination[band],
                            src_transform=source.transform, src_crs=source.crs,
                            src_nodata=source.nodata,
                            dst_transform=transform, dst_crs=TARGET_CRS,
                            dst_nodata=np.nan, resampling=Resampling.bilinear,
                            init_dest_nodata=False)
            except rasterio.errors.RasterioIOError as error:
                if "401" in str(error):
                    raise RuntimeError(
                        "Lantmäteriet rejected the COG download (401). Order free API "
                        "access in Geotorget and set LANTMATERIET_USERNAME and "
                        "LANTMATERIET_PASSWORD.") from error
                raise
    if np.isnan(destination).all():
        raise RuntimeError("downloaded assets did not cover the requested output")
    return destination, transform


def save_heightmap(elevation: np.ndarray, path: Path) -> tuple[float, float]:
    valid = elevation[np.isfinite(elevation)]
    minimum, maximum = float(valid.min()), float(valid.max())
    filled = np.where(np.isfinite(elevation), elevation, minimum)
    height_range = max(1.0, maximum - minimum)
    encoded = np.rint(np.clip((filled - minimum) / height_range, 0, 1) * 65535).astype("<u2")
    Image.frombytes("I;16", (encoded.shape[1], encoded.shape[0]), encoded.tobytes()).save(path)
    return minimum, height_range


def save_aerial(rgb: np.ndarray, path: Path) -> None:
    image = np.moveaxis(rgb[:3], 0, 2)
    image = np.nan_to_num(image, nan=0.0)
    if image.max() <= 1.0:
        image *= 255.0
    Image.fromarray(np.clip(image, 0, 255).astype(np.uint8), "RGB").save(path, optimize=True)


def save_normal_map(elevation: np.ndarray, size_x: float, size_y: float, path: Path) -> None:
    filled = np.where(np.isfinite(elevation), elevation, np.nanmin(elevation))
    dz, dx = np.gradient(filled, size_y / elevation.shape[0], size_x / elevation.shape[1])
    normal = np.dstack((-dx, np.ones_like(dx), dz))
    normal /= np.linalg.norm(normal, axis=2, keepdims=True)
    Image.fromarray(np.clip((normal * .5 + .5) * 255, 0, 255).astype(np.uint8), "RGB").save(path)


def write_world(source: Path, name: str, bbox: tuple[float, float, float, float],
                size_x: float, size_y: float, minimum: float, height_range: float) -> Path:
    latitude = (bbox[1] + bbox[3]) * .5
    longitude = (bbox[0] + bbox[2]) * .5
    world = source / f"{name}.world"
    world.write_text(f"""<?xml version="1.0"?>
<sdf version="1.9"><world name="{name}">
  <spherical_coordinates><surface_model>EARTH_WGS84</surface_model>
    <latitude_deg>{latitude}</latitude_deg><longitude_deg>{longitude}</longitude_deg>
    <elevation>{minimum}</elevation></spherical_coordinates>
  <model name="{name}"><static>true</static><link name="ground">
    <visual name="ground_visual"><geometry><heightmap>
      <texture><diffuse>mesh/aerial.png</diffuse><normal>mesh/normal_map.png</normal>
        <size>{size_x}</size></texture>
      <uri>mesh/height_map.png</uri><size>{size_x} {size_y} {height_range}</size>
      <pos>0 0 0</pos><sampling>1</sampling>
    </heightmap></geometry></visual>
  </link></model>
</world></sdf>
""", encoding="utf-8")
    return world


def generate(args: argparse.Namespace) -> None:
    bbox = tuple(args.bounds)
    username = os.environ.get(args.username_env)
    password = os.environ.get(args.password_env)
    height_assets = newest_collection(discover_assets(HEIGHT_STAC, bbox))
    image_assets = newest_collection(discover_assets(IMAGE_STAC, bbox))
    print(f"Found {len(height_assets)} elevation and {len(image_assets)} orthophoto assets")
    if args.discover_only:
        print(json.dumps({"elevation": [asset.__dict__ for asset in height_assets],
                          "orthophoto": [asset.__dict__ for asset in image_assets]}, indent=2))
        return

    source = args.output / "source"
    mesh = source / "mesh"
    mesh.mkdir(parents=True, exist_ok=True)
    transformer = Transformer.from_crs("EPSG:4326", TARGET_CRS, always_xy=True)
    west, south = transformer.transform(bbox[0], bbox[1])
    east, north = transformer.transform(bbox[2], bbox[3])
    size_x, size_y = east - west, north - south
    elevation, _ = mosaic(height_assets, bbox, args.height_size, args.height_size, 1,
                          username, password)
    aerial, _ = mosaic(image_assets, bbox, args.texture_size, args.texture_size, 3,
                       username, password)
    minimum, height_range = save_heightmap(elevation[0], mesh / "height_map.png")
    save_aerial(aerial, mesh / "aerial.png")
    save_normal_map(elevation[0], size_x, size_y, mesh / "normal_map.png")
    world = write_world(source, args.name, bbox, size_x, size_y, minimum, height_range)

    spec = import_gazebo_terrain.read_spec(world)
    import_gazebo_terrain.write_terrain(
        spec, args.output, args.name, args.grid_size, args.metres_per_unit,
        args.texture_size)
    metadata = {
        "name": args.name, "boundsWgs84": bbox, "crs": TARGET_CRS,
        "sizeMetres": [size_x, size_y, height_range],
        "elevationMetres": [minimum, minimum + height_range],
        "terrain": f"{args.name}_terrain.gltf",
        "source": "Lantmäteriet", "license": "CC BY 4.0",
        "elevationItems": [asset.item_id for asset in height_assets],
        "orthophotoItems": [asset.item_id for asset in image_assets],
        "catalogs": [HEIGHT_STAC, IMAGE_STAC],
    }
    (args.output / f"{args.name}.lantmateriet.json").write_text(
        json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    print(f"Generated {args.output / (args.name + '_terrain.gltf')}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bounds", type=float, nargs=4, metavar=("WEST", "SOUTH", "EAST", "NORTH"), required=True)
    parser.add_argument("--name", required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--height-size", type=int, default=1025)
    parser.add_argument("--texture-size", type=int, default=2048)
    parser.add_argument("--grid-size", type=int, default=385)
    parser.add_argument("--metres-per-unit", type=float, default=200.0)
    parser.add_argument("--username-env", default="LANTMATERIET_USERNAME")
    parser.add_argument("--password-env", default="LANTMATERIET_PASSWORD")
    parser.add_argument("--discover-only", action="store_true")
    args = parser.parse_args()
    if args.bounds[0] >= args.bounds[2] or args.bounds[1] >= args.bounds[3]:
        parser.error("bounds must be WEST SOUTH EAST NORTH")
    try:
        generate(args)
        return 0
    except RuntimeError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
