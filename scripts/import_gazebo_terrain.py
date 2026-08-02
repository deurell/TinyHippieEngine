#!/usr/bin/env python3
"""Convert gazebo_terrain_generator output into Tiny Hippie glTF assets.

The importer reads the visual <heightmap> from the generated SDF / world file,
uses its physical size to decode the normalized 8- or 16-bit height image, and
optionally drapes a GeoJSON, GPX, or Overpass JSON route over the terrain.
"""

from __future__ import annotations

import argparse
import json
import math
import shutil
import struct
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path

from PIL import Image


@dataclass(frozen=True)
class TerrainSpec:
    sdf: Path
    heightmap: Path
    aerial: Path | None
    size_x: float
    size_y: float
    size_z: float
    pos_x: float
    pos_y: float
    pos_z: float
    latitude: float | None
    longitude: float | None


def _numbers(text: str | None, count: int, field: str) -> list[float]:
    values = [float(value) for value in (text or "").split()]
    if len(values) < count:
        raise ValueError(f"{field} must contain at least {count} numbers")
    return values


def _resolve_uri(sdf: Path, uri: str) -> Path:
    if uri.startswith("file://"):
        return Path(uri[7:])
    if uri.startswith("model://"):
        uri = uri[len("model://"):]
        parts = Path(uri).parts
        if parts and parts[0] == sdf.parent.name:
            uri = str(Path(*parts[1:]))
    return (sdf.parent / uri).resolve()


def find_sdf(source: Path) -> Path:
    if source.is_file():
        return source.resolve()
    candidates = sorted(source.glob("*.world")) + sorted(source.glob("*.sdf"))
    if not candidates:
        raise ValueError(f"no .world or .sdf file found in {source}")
    for candidate in candidates:
        try:
            if ET.parse(candidate).find(".//visual/geometry/heightmap") is not None:
                return candidate.resolve()
        except ET.ParseError:
            pass
    raise ValueError(f"no visual heightmap found in SDF files under {source}")


def read_spec(source: Path) -> TerrainSpec:
    sdf = find_sdf(source)
    root = ET.parse(sdf).getroot()
    heightmap = root.find(".//visual/geometry/heightmap")
    if heightmap is None:
        heightmap = root.find(".//heightmap")
    if heightmap is None:
        raise ValueError(f"no <heightmap> found in {sdf}")
    uri = heightmap.findtext("uri")
    if not uri:
        raise ValueError(f"heightmap in {sdf} has no <uri>")
    size = _numbers(heightmap.findtext("size"), 3, "heightmap size")
    pos = _numbers(heightmap.findtext("pos") or "0 0 0", 3, "heightmap pos")
    diffuse = heightmap.findtext("texture/diffuse")
    spherical = root.find(".//spherical_coordinates")
    latitude = longitude = None
    if spherical is not None:
        latitude_text = spherical.findtext("latitude_deg")
        longitude_text = spherical.findtext("longitude_deg")
        if latitude_text is not None and longitude_text is not None:
            latitude, longitude = float(latitude_text), float(longitude_text)
    return TerrainSpec(
        sdf=sdf,
        heightmap=_resolve_uri(sdf, uri),
        aerial=_resolve_uri(sdf, diffuse) if diffuse else None,
        size_x=size[0], size_y=size[1], size_z=size[2],
        pos_x=pos[0], pos_y=pos[1], pos_z=pos[2],
        latitude=latitude, longitude=longitude,
    )


def decode_heightmap(path: Path, size_z: float, grid_size: int) -> Image.Image:
    if not path.is_file():
        raise ValueError(f"heightmap does not exist: {path}")
    with Image.open(path) as source:
        extrema = source.getextrema()
        if isinstance(extrema[0], tuple):
            raise ValueError(f"heightmap must be a one-channel image: {path}")
        # Gazebo heightmaps are normalized across the storage bit depth. PIL
        # exposes 16-bit PNGs as I / I;16 and 8-bit PNGs as L.
        maximum = 255.0 if source.mode in ("1", "L", "P") else 65535.0
        floating = source.convert("F")
        floating = floating.resize((grid_size, grid_size), Image.Resampling.BICUBIC)
        decoded = Image.new("F", floating.size)
        decoded.putdata([max(0.0, min(maximum, value)) * size_z / maximum
                         for value in floating.getdata()])
        return decoded


def _append_chunk(binary: bytearray, chunk: bytes) -> int:
    while len(binary) % 4:
        binary.append(0)
    offset = len(binary)
    binary.extend(chunk)
    return offset


def write_terrain(spec: TerrainSpec, output: Path, name: str, grid_size: int,
                  metres_per_unit: float, texture_max_size: int = 2048) -> Image.Image:
    heights_image = decode_heightmap(spec.heightmap, spec.size_z, grid_size)
    heights = list(heights_image.getdata())
    dx = spec.size_x / (grid_size - 1)
    dz = spec.size_y / (grid_size - 1)
    positions: list[float] = []
    normals: list[float] = []
    uvs: list[float] = []
    for row in range(grid_size):
        for column in range(grid_size):
            left = heights[row * grid_size + max(0, column - 1)]
            right = heights[row * grid_size + min(grid_size - 1, column + 1)]
            north = heights[max(0, row - 1) * grid_size + column]
            south = heights[min(grid_size - 1, row + 1) * grid_size + column]
            sx = dx * (2 if 0 < column < grid_size - 1 else 1)
            sz = dz * (2 if 0 < row < grid_size - 1 else 1)
            nx, ny, nz = -(right - left) / sx, 1.0, -(south - north) / sz
            length = math.sqrt(nx * nx + ny * ny + nz * nz)
            positions.extend(((column / (grid_size - 1) - .5) * spec.size_x / metres_per_unit,
                              heights[row * grid_size + column] / metres_per_unit,
                              (row / (grid_size - 1) - .5) * spec.size_y / metres_per_unit))
            normals.extend((nx / length, ny / length, nz / length))
            uvs.extend((column / (grid_size - 1), 1.0 - row / (grid_size - 1)))
    indices: list[int] = []
    for row in range(grid_size - 1):
        for column in range(grid_size - 1):
            a = row * grid_size + column
            indices.extend((a, a + grid_size, a + 1,
                            a + 1, a + grid_size, a + grid_size + 1))
    chunks = [struct.pack(f"<{len(positions)}f", *positions),
              struct.pack(f"<{len(normals)}f", *normals),
              struct.pack(f"<{len(uvs)}f", *uvs),
              struct.pack(f"<{len(indices)}I", *indices)]
    binary = bytearray()
    offsets = [_append_chunk(binary, chunk) for chunk in chunks]
    bin_name = f"{name}_terrain.bin"
    texture_name = f"{name}_aerial{spec.aerial.suffix.lower() if spec.aerial else '.png'}"
    (output / bin_name).write_bytes(binary)
    if spec.aerial and spec.aerial.is_file():
        with Image.open(spec.aerial) as aerial:
            if max(aerial.size) > texture_max_size:
                aerial.thumbnail((texture_max_size, texture_max_size), Image.Resampling.LANCZOS)
                aerial.convert("RGB").save(output / texture_name, optimize=True)
            else:
                shutil.copyfile(spec.aerial, output / texture_name)
    else:
        Image.new("RGB", (1, 1), (105, 118, 90)).save(output / texture_name)
    vertex_count = grid_size * grid_size
    half_x, half_z = spec.size_x * .5 / metres_per_unit, spec.size_y * .5 / metres_per_unit
    gltf = {
        "asset": {"version": "2.0", "generator": "Tiny Hippie Gazebo terrain importer"},
        "scene": 0, "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0, "name": name}],
        "meshes": [{"primitives": [{"attributes": {"POSITION": 0, "NORMAL": 1, "TEXCOORD_0": 2},
                                      "indices": 3, "material": 0}]}],
        "materials": [{"name": "aerial terrain", "pbrMetallicRoughness": {
            "baseColorTexture": {"index": 0}, "metallicFactor": 0.0, "roughnessFactor": .92}}],
        "textures": [{"source": 0}], "images": [{"uri": texture_name}],
        "buffers": [{"uri": bin_name, "byteLength": len(binary)}],
        "bufferViews": [{"buffer": 0, "byteOffset": offsets[i], "byteLength": len(chunks[i]),
                         "target": 34963 if i == 3 else 34962} for i in range(4)],
        "accessors": [
            {"bufferView": 0, "componentType": 5126, "count": vertex_count, "type": "VEC3",
             "min": [-half_x, 0.0, -half_z], "max": [half_x, spec.size_z / metres_per_unit, half_z]},
            {"bufferView": 1, "componentType": 5126, "count": vertex_count, "type": "VEC3"},
            {"bufferView": 2, "componentType": 5126, "count": vertex_count, "type": "VEC2"},
            {"bufferView": 3, "componentType": 5125, "count": len(indices), "type": "SCALAR"}],
    }
    (output / f"{name}_terrain.gltf").write_text(json.dumps(gltf, indent=2) + "\n")
    return heights_image


def load_route(path: Path) -> list[tuple[float, float]]:
    if path.suffix.lower() == ".gpx":
        root = ET.parse(path).getroot()
        return [(float(point.attrib["lat"]), float(point.attrib["lon"]))
                for point in root.iter() if point.tag.rsplit("}", 1)[-1] in ("trkpt", "rtept")]
    data = json.loads(path.read_text())
    if data.get("type") == "FeatureCollection":
        features = data["features"]
        geometry = next(feature["geometry"] for feature in features
                        if feature.get("geometry", {}).get("type") in ("LineString", "MultiLineString"))
        coordinates = geometry["coordinates"]
        if geometry["type"] == "MultiLineString":
            coordinates = [point for line in coordinates for point in line]
        return [(float(lat), float(lon)) for lon, lat, *_ in coordinates]
    if data.get("type") == "LineString":
        return [(float(lat), float(lon)) for lon, lat, *_ in data["coordinates"]]
    elements = data.get("elements", [])
    relation = next((element for element in elements if element.get("type") == "relation"), None)
    if relation:
        segments = [[(point["lat"], point["lon"]) for point in member.get("geometry", [])]
                    for member in relation["members"] if member.get("geometry")]
        route = list(segments[0])
        for segment in segments[1:]:
            if math.dist(route[-1], segment[-1]) < math.dist(route[-1], segment[0]):
                segment.reverse()
            route.extend(segment)
        return route
    raise ValueError(f"unsupported route format: {path}")


def _bilinear(image: Image.Image, x: float, y: float) -> float:
    x, y = max(0., min(image.width - 1.001, x)), max(0., min(image.height - 1.001, y))
    x0, y0 = int(x), int(y)
    tx, ty = x - x0, y - y0
    return ((image.getpixel((x0, y0)) * (1 - tx) + image.getpixel((x0 + 1, y0)) * tx) * (1 - ty) +
            (image.getpixel((x0, y0 + 1)) * (1 - tx) + image.getpixel((x0 + 1, y0 + 1)) * tx) * ty)


def route_points(spec: TerrainSpec, heights: Image.Image, route: Path,
                 metres_per_unit: float) -> list[tuple[float, float, float]]:
    if spec.latitude is None or spec.longitude is None:
        raise ValueError("route import requires <spherical_coordinates> in the SDF")
    radius = 6378137.0
    result = []
    for latitude, longitude in load_route(route):
        east = math.radians(longitude - spec.longitude) * radius * math.cos(math.radians(spec.latitude))
        north = math.radians(latitude - spec.latitude) * radius
        local_x, local_north = east - spec.pos_x, north - spec.pos_y
        px = (local_x / spec.size_x + .5) * (heights.width - 1)
        py = (.5 - local_north / spec.size_y) * (heights.height - 1)
        if 0 <= px < heights.width - 1 and 0 <= py < heights.height - 1:
            result.append((local_x / metres_per_unit,
                           _bilinear(heights, px, py) / metres_per_unit,
                           -local_north / metres_per_unit))
    if len(result) < 2:
        raise ValueError("route has fewer than two points inside the terrain")
    return result


def write_route(points: list[tuple[float, float, float]], output: Path, name: str,
                metres_per_unit: float) -> None:
    sides, radius, lift = 10, 15.0 / metres_per_unit, 8.0 / metres_per_unit
    positions: list[float] = []
    normals: list[float] = []
    for index, point in enumerate(points):
        previous, following = points[max(0, index - 1)], points[min(len(points) - 1, index + 1)]
        dx, dz = following[0] - previous[0], following[2] - previous[2]
        length = math.hypot(dx, dz) or 1.0
        side_x, side_z = -dz / length, dx / length
        for side in range(sides):
            angle = side * math.tau / sides
            c, s = math.cos(angle), math.sin(angle)
            positions.extend((point[0] + side_x * radius * c,
                              point[1] + lift + radius * .45 * s,
                              point[2] + side_z * radius * c))
            normals.extend((side_x * c, s, side_z * c))
    indices: list[int] = []
    for ring in range(len(points) - 1):
        for side in range(sides):
            a, b = ring * sides + side, ring * sides + (side + 1) % sides
            c, d = a + sides, b + sides
            indices.extend((a, c, b, b, c, d))
    chunks = [struct.pack(f"<{len(positions)}f", *positions),
              struct.pack(f"<{len(normals)}f", *normals),
              struct.pack(f"<{len(indices)}I", *indices)]
    binary = bytearray()
    offsets = [_append_chunk(binary, chunk) for chunk in chunks]
    bin_name = f"{name}_route.bin"
    (output / bin_name).write_bytes(binary)
    gltf = {
        "asset": {"version": "2.0", "generator": "Tiny Hippie route importer"},
        "scene": 0, "scenes": [{"nodes": [0]}], "nodes": [{"mesh": 0, "name": f"{name} route"}],
        "meshes": [{"primitives": [{"attributes": {"POSITION": 0, "NORMAL": 1}, "indices": 2, "material": 0}]}],
        "materials": [{"name": "navigation orange", "pbrMetallicRoughness": {
            "baseColorFactor": [1.0, .16, .015, 1.0], "metallicFactor": 0.0, "roughnessFactor": .38}}],
        "buffers": [{"uri": bin_name, "byteLength": len(binary)}],
        "bufferViews": [{"buffer": 0, "byteOffset": offsets[i], "byteLength": len(chunks[i]),
                         "target": 34963 if i == 2 else 34962} for i in range(3)],
        "accessors": [{"bufferView": 0, "componentType": 5126, "count": len(positions) // 3, "type": "VEC3"},
                      {"bufferView": 1, "componentType": 5126, "count": len(normals) // 3, "type": "VEC3"},
                      {"bufferView": 2, "componentType": 5125, "count": len(indices), "type": "SCALAR"}]}
    (output / f"{name}_route.gltf").write_text(json.dumps(gltf, indent=2) + "\n")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="generated folder, .world, or .sdf")
    parser.add_argument("output", type=Path)
    parser.add_argument("--name", default=None, help="output basename (default: source folder name)")
    parser.add_argument("--route", type=Path, help="GeoJSON, GPX, or Overpass JSON route")
    parser.add_argument("--grid-size", type=int, default=385)
    parser.add_argument("--metres-per-unit", type=float, default=200.0)
    parser.add_argument("--texture-max-size", type=int, default=2048,
                        help="maximum aerial texture edge for web output")
    args = parser.parse_args()
    if args.grid_size < 2 or args.metres_per_unit <= 0 or args.texture_max_size < 1:
        parser.error("grid and texture sizes must be positive; --grid-size must be >= 2")
    spec = read_spec(args.source)
    name = args.name or spec.sdf.parent.name.lower().replace(" ", "_")
    args.output.mkdir(parents=True, exist_ok=True)
    heights = write_terrain(spec, args.output, name, args.grid_size,
                            args.metres_per_unit, args.texture_max_size)
    if args.route:
        write_route(route_points(spec, heights, args.route, args.metres_per_unit),
                    args.output, name, args.metres_per_unit)
    manifest = {"name": name, "source": str(spec.sdf),
                "terrain": f"{name}_terrain.gltf",
                "route": f"{name}_route.gltf" if args.route else None,
                "metresPerUnit": args.metres_per_unit,
                "sizeMetres": [spec.size_x, spec.size_y, spec.size_z],
                "origin": {"latitude": spec.latitude, "longitude": spec.longitude}}
    (args.output / f"{name}.terrain.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"Imported {spec.sdf} -> {args.output / (name + '_terrain.gltf')}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
