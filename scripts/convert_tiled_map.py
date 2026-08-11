#!/usr/bin/env python3
"""Convert a Tiled TMX map into Tiny Hippie Engine scene JSON.

The output is a complete generated scene and is replaced on every conversion.
"""

from __future__ import annotations

import argparse
import csv
import json
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


def repo_relative(path: Path, repo_root: Path) -> str:
    try:
        return path.resolve().relative_to(repo_root.resolve()).as_posix()
    except ValueError:
        return path.as_posix()


def parse_csv_data(text: str) -> list[int]:
    values: list[int] = []
    for row in csv.reader(line for line in text.strip().splitlines() if line.strip()):
        values.extend(int(cell) for cell in row if cell != "")
    return values


def parse_layer_z(values: list[str]) -> dict[str, float]:
    result: dict[str, float] = {}
    for value in values:
        if "=" not in value:
            raise ValueError(f"layer z override must be NAME=VALUE, got {value!r}")
        name, z_value = value.split("=", 1)
        result[name] = float(z_value)
    return result


def infer_packed_image(image_path: Path) -> Path:
    packed = image_path.with_name(f"{image_path.stem}_packed{image_path.suffix}")
    return packed if packed.exists() else image_path


def load_tileset(tmx_path: Path, tileset_element: ET.Element, use_packed: bool) -> dict:
    source = tileset_element.get("source")
    if source is None:
        tileset_root = tileset_element
        tsx_path = tmx_path
    else:
        tsx_path = (tmx_path.parent / source).resolve()
        tileset_root = ET.parse(tsx_path).getroot()

    image_element = tileset_root.find("image")
    if image_element is None or image_element.get("source") is None:
        raise ValueError("tileset must contain an image source")

    image_path = (tsx_path.parent / image_element.get("source")).resolve()
    if use_packed:
        image_path = infer_packed_image(image_path)

    return {
        "firstgid": int(tileset_element.get("firstgid", "1")),
        "tileWidth": int(tileset_root.get("tilewidth", "0")),
        "tileHeight": int(tileset_root.get("tileheight", "0")),
        "columns": int(tileset_root.get("columns", "1")),
        "image": image_path,
    }


def scene_name_from_path(path: Path) -> str:
    stem = re.sub(r"[^a-zA-Z0-9]+", "_", path.stem).strip("_").lower()
    return stem or "tilemap_scene"


def build_scene(args: argparse.Namespace) -> dict:
    repo_root = Path(args.repo_root).resolve()
    tmx_path = Path(args.input).resolve()
    root = ET.parse(tmx_path).getroot()
    if root.tag != "map":
        raise ValueError(f"expected a Tiled map root, got {root.tag!r}")
    if root.get("orientation") != "orthogonal":
        raise ValueError("only orthogonal Tiled maps are supported")

    tileset_elements = root.findall("tileset")
    if len(tileset_elements) != 1:
        raise ValueError("only one tileset per map is currently supported")
    tileset = load_tileset(tmx_path, tileset_elements[0], args.use_packed)

    map_width = int(root.get("width", "0"))
    map_height = int(root.get("height", "0"))
    layer_z = parse_layer_z(args.layer_z)

    layers: list[dict] = []
    for index, layer in enumerate(root.findall("layer")):
        name = layer.get("name", f"Layer {index}")
        data_element = layer.find("data")
        if data_element is None or data_element.get("encoding") != "csv":
            raise ValueError(f"layer {name!r} must use CSV data encoding")
        data = parse_csv_data(data_element.text or "")
        expected_count = map_width * map_height
        if len(data) != expected_count:
            raise ValueError(
                f"layer {name!r} has {len(data)} tiles, expected {expected_count}"
            )
        layers.append({"name": name, "z": layer_z.get(name, index * args.layer_step), "data": data})

    nodes: list[dict] = [
        {
            "name": args.camera_name,
            "type": "CameraNode",
            "active": True,
            "fov": args.camera_fov,
            "lookAt": args.camera_look_at,
            "transform": {"position": args.camera_position},
        }
    ]
    if args.hero_marker:
        nodes.append(
            {
                "name": args.hero_marker,
                "type": "SceneNode",
                "transform": {"position": [0.0, 0.0, 0.0]},
            }
        )

    nodes.append(
        {
            "name": args.node_name,
            "type": "TileMapNode",
            "tileMap": {
                "image": repo_relative(tileset["image"], repo_root),
                "firstGid": tileset["firstgid"],
                "mapWidth": map_width,
                "mapHeight": map_height,
                "tileWidth": tileset["tileWidth"],
                "tileHeight": tileset["tileHeight"],
                "columns": tileset["columns"],
                "tileWorldSize": args.tile_world_size,
                "layers": layers,
            },
        }
    )

    return {"name": args.scene_name or scene_name_from_path(tmx_path), "nodes": nodes}


def parse_vec3(value: str) -> list[float]:
    parts = [part.strip() for part in value.split(",")]
    if len(parts) != 3:
        raise argparse.ArgumentTypeError("expected three comma-separated numbers")
    return [float(part) for part in parts]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", help="Input Tiled .tmx file")
    parser.add_argument("output", help="Output scene .json file (overwritten)")
    parser.add_argument("--repo-root", default=".", help="Repository root for resource-relative paths")
    parser.add_argument("--scene-name", default="", help="Scene name in generated JSON")
    parser.add_argument("--node-name", default="tile_map", help="Generated TileMapNode name")
    parser.add_argument("--tile-world-size", type=float, default=1.0)
    parser.add_argument("--layer-z", action="append", default=[], help="Layer z override as NAME=VALUE")
    parser.add_argument("--layer-step", type=float, default=0.01, help="Fallback z spacing per layer")
    parser.add_argument("--use-packed", action="store_true", help="Use *_packed image when present")
    parser.add_argument("--camera-name", default="main_camera")
    parser.add_argument("--camera-position", type=parse_vec3, default=[0.0, 0.0, 10.0])
    parser.add_argument("--camera-look-at", type=parse_vec3, default=[0.0, 0.0, 0.0])
    parser.add_argument("--camera-fov", type=float, default=38.0)
    parser.add_argument("--hero-marker", default="", help="Optional marker node name")
    args = parser.parse_args()

    try:
        scene = build_scene(args)
        output = Path(args.output)
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(scene, indent=2) + "\n", encoding="utf-8")
    except Exception as exc:
        print(f"convert_tiled_map.py: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
