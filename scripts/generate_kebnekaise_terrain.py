#!/usr/bin/env python3
"""Build the checked-in Kebnekaise web terrain assets from Terrarium tiles."""

import argparse
import json
import math
import struct
from pathlib import Path

from PIL import Image, ImageFilter

import import_gazebo_terrain as gazebo_importer


ZOOM = 12
TILE_X = range(2257, 2260)
TILE_Y = range(982, 985)
GRID_SIZE = 385
TEXTURE_SIZE = 1024
METERS_PER_WORLD_UNIT = 200.0


def emit_gazebo_source(elevation_image: Image.Image, output_dir: Path) -> Path:
    """Create the same SDF/heightmap/aerial contract as the external tool."""
    source_dir = output_dir / "GazeboSource"
    mesh_dir = source_dir / "mesh"
    mesh_dir.mkdir(parents=True, exist_ok=True)
    gazebo_size = 1025
    sampled = elevation_image.resize(
        (gazebo_size, gazebo_size), Image.Resampling.BICUBIC)
    heights = list(sampled.getdata())
    minimum, maximum = min(heights), max(heights)
    height_range = max(1.0, maximum - minimum)
    encoded = [round(max(0.0, min(1.0, (height - minimum) / height_range)) * 65535)
               for height in heights]
    Image.frombytes("I;16", sampled.size,
                    struct.pack(f"<{len(encoded)}H", *encoded)).save(
                        mesh_dir / "height_map.png")
    write_texture(elevation_image, mesh_dir / "aerial.png")

    center_x = (min(TILE_X) * 256 + max(TILE_X) * 256 + 255) * .5
    center_y = (min(TILE_Y) * 256 + max(TILE_Y) * 256 + 255) * .5
    scale = 256 * (1 << ZOOM)
    center_lon = center_x / scale * 360.0 - 180.0
    center_lat = math.degrees(math.atan(math.sinh(
        math.pi * (1.0 - 2.0 * center_y / scale))))
    metres_per_pixel = (2.0 * math.pi * 6378137.0 * math.cos(math.radians(center_lat))
                        / scale)
    size_x = elevation_image.width * metres_per_pixel
    size_y = elevation_image.height * metres_per_pixel
    (source_dir / "Kebnekaise.world").write_text(f"""<?xml version="1.0"?>
<sdf version="1.9"><world name="kebnekaise">
  <spherical_coordinates><surface_model>EARTH_WGS84</surface_model>
    <latitude_deg>{center_lat}</latitude_deg><longitude_deg>{center_lon}</longitude_deg>
    <elevation>{minimum}</elevation></spherical_coordinates>
  <model name="Kebnekaise"><static>true</static><link name="ground">
    <visual name="ground_visual"><geometry><heightmap>
      <texture><diffuse>mesh/aerial.png</diffuse><size>{size_x}</size></texture>
      <uri>mesh/height_map.png</uri><size>{size_x} {size_y} {height_range}</size>
      <pos>0 0 0</pos><sampling>1</sampling>
    </heightmap></geometry></visual>
  </link></model>
</world></sdf>
""", encoding="utf-8")
    return source_dir
def global_pixel(lat: float, lon: float) -> tuple[float, float]:
    scale = 256 * (1 << ZOOM)
    x = (lon + 180.0) / 360.0 * scale
    lat_rad = math.radians(lat)
    y = (1.0 - math.asinh(math.tan(lat_rad)) / math.pi) * 0.5 * scale
    return x, y


def elevation(pixel: tuple[int, int, int]) -> float:
    red, green, blue = pixel
    return red * 256.0 + green + blue / 256.0 - 32768.0


def decode_elevation_image(encoded: Image.Image) -> Image.Image:
    decoded = Image.new("F", encoded.size)
    decoded.putdata([elevation(pixel) for pixel in encoded.getdata()])
    return decoded


def stitch_tiles(tile_dir: Path) -> Image.Image:
    mosaic = Image.new("RGB", (len(TILE_X) * 256, len(TILE_Y) * 256))
    for row, tile_y in enumerate(TILE_Y):
        for column, tile_x in enumerate(TILE_X):
            path = tile_dir / f"{tile_x}_{tile_y}.png"
            with Image.open(path) as tile:
                mosaic.paste(tile.convert("RGB"), (column * 256, row * 256))
    return mosaic


def bilinear_height(image: Image.Image, x: float, y: float) -> float:
    x = max(0.0, min(x, image.width - 1.001))
    y = max(0.0, min(y, image.height - 1.001))
    x0, y0 = int(x), int(y)
    tx, ty = x - x0, y - y0
    h00 = float(image.getpixel((x0, y0)))
    h10 = float(image.getpixel((x0 + 1, y0)))
    h01 = float(image.getpixel((x0, y0 + 1)))
    h11 = float(image.getpixel((x0 + 1, y0 + 1)))
    return (h00 * (1.0 - tx) + h10 * tx) * (1.0 - ty) + (
        h01 * (1.0 - tx) + h11 * tx
    ) * ty


def terrain_color(height: float, shade: float) -> tuple[int, int, int]:
    if height < 1050.0:
        low, high, blend = (47, 74, 54), (75, 92, 59), (height - 700.0) / 350.0
    elif height < 1450.0:
        low, high, blend = (75, 92, 59), (111, 105, 83), (height - 1050.0) / 400.0
    elif height < 1800.0:
        low, high, blend = (111, 105, 83), (139, 139, 134), (height - 1450.0) / 350.0
    else:
        low, high, blend = (139, 139, 134), (226, 235, 239), (height - 1800.0) / 350.0
    blend = max(0.0, min(blend, 1.0))
    return tuple(
        max(0, min(255, round((a + (b - a) * blend) * shade)))
        for a, b in zip(low, high)
    )


def write_texture(elevation_image: Image.Image, output: Path) -> None:
    sampled = elevation_image.resize(
        (TEXTURE_SIZE, TEXTURE_SIZE), Image.Resampling.BICUBIC)
    heights = list(sampled.getdata())
    texture = Image.new("RGB", sampled.size)
    pixels = []
    width = sampled.width
    for index, height in enumerate(heights):
        x, y = index % width, index // width
        left = heights[y * width + max(x - 1, 0)]
        right = heights[y * width + min(x + 1, width - 1)]
        up = heights[max(y - 1, 0) * width + x]
        down = heights[min(y + 1, width - 1) * width + x]
        shade = max(0.62, min(1.28, 1.0 + (left - right + down - up) * 0.0025))
        pixels.append(terrain_color(height, shade))
    texture.putdata(pixels)
    texture.filter(ImageFilter.GaussianBlur(0.35)).save(output, optimize=True)


def write_terrain(
    elevation_image: Image.Image, output_dir: Path
) -> tuple[float, float, Image.Image]:
    grid = elevation_image.resize(
        (GRID_SIZE, GRID_SIZE), Image.Resampling.BICUBIC)
    heights = list(grid.getdata())
    base_height = min(heights)
    center_lat = 67.9006
    meters_per_pixel = (
        2.0 * math.pi * 6378137.0 * math.cos(math.radians(center_lat))
        / ((1 << ZOOM) * 256.0)
    )
    spacing = meters_per_pixel * (elevation_image.width - 1) / (GRID_SIZE - 1)
    positions = []
    normals = []
    uvs = []
    for row in range(GRID_SIZE):
        for column in range(GRID_SIZE):
            left = heights[row * GRID_SIZE + max(column - 1, 0)]
            right = heights[row * GRID_SIZE + min(column + 1, GRID_SIZE - 1)]
            north = heights[max(row - 1, 0) * GRID_SIZE + column]
            south = heights[min(row + 1, GRID_SIZE - 1) * GRID_SIZE + column]
            dx = spacing * (2.0 if 0 < column < GRID_SIZE - 1 else 1.0)
            dz = spacing * (2.0 if 0 < row < GRID_SIZE - 1 else 1.0)
            nx, ny, nz = -(right - left) / dx, 1.0, -(south - north) / dz
            normal_length = math.sqrt(nx * nx + ny * ny + nz * nz)
            positions.extend((
                (column - (GRID_SIZE - 1) * 0.5) * spacing / METERS_PER_WORLD_UNIT,
                (heights[row * GRID_SIZE + column] - base_height) / METERS_PER_WORLD_UNIT,
                (row - (GRID_SIZE - 1) * 0.5) * spacing / METERS_PER_WORLD_UNIT,
            ))
            normals.extend((nx / normal_length, ny / normal_length, nz / normal_length))
            uvs.extend((column / (GRID_SIZE - 1), 1.0 - row / (GRID_SIZE - 1)))
    indices = []
    for row in range(GRID_SIZE - 1):
        for column in range(GRID_SIZE - 1):
            a = row * GRID_SIZE + column
            b, c, d = a + 1, a + GRID_SIZE, a + GRID_SIZE + 1
            indices.extend((a, c, b, b, c, d))

    chunks = (
        struct.pack(f"<{len(positions)}f", *positions),
        struct.pack(f"<{len(normals)}f", *normals),
        struct.pack(f"<{len(uvs)}f", *uvs),
        struct.pack(f"<{len(indices)}I", *indices),
    )
    offsets = []
    binary = bytearray()
    for chunk in chunks:
        while len(binary) % 4:
            binary.append(0)
        offsets.append(len(binary))
        binary.extend(chunk)
    (output_dir / "kebnekaise_terrain.bin").write_bytes(binary)
    half_extent = spacing * (GRID_SIZE - 1) * 0.5 / METERS_PER_WORLD_UNIT
    max_height = (max(heights) - base_height) / METERS_PER_WORLD_UNIT
    vertex_count = GRID_SIZE * GRID_SIZE
    gltf = {
        "asset": {"version": "2.0", "generator": "Tiny Hippie Kebnekaise generator"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0, "name": "Kebnekaise terrain"}],
        "meshes": [{"primitives": [{
            "attributes": {"POSITION": 0, "NORMAL": 1, "TEXCOORD_0": 2},
            "indices": 3,
            "material": 0,
        }]}],
        "materials": [{
            "name": "terrain",
            "pbrMetallicRoughness": {
                "baseColorTexture": {"index": 0},
                "metallicFactor": 0.0,
                "roughnessFactor": 0.92,
            },
        }],
        "textures": [{"source": 0}],
        "images": [{"uri": "kebnekaise_terrain.png"}],
        "buffers": [{"uri": "kebnekaise_terrain.bin", "byteLength": len(binary)}],
        "bufferViews": [
            {"buffer": 0, "byteOffset": offsets[0], "byteLength": len(chunks[0]), "target": 34962},
            {"buffer": 0, "byteOffset": offsets[1], "byteLength": len(chunks[1]), "target": 34962},
            {"buffer": 0, "byteOffset": offsets[2], "byteLength": len(chunks[2]), "target": 34962},
            {"buffer": 0, "byteOffset": offsets[3], "byteLength": len(chunks[3]), "target": 34963},
        ],
        "accessors": [
            {"bufferView": 0, "componentType": 5126, "count": vertex_count,
             "type": "VEC3", "min": [-half_extent, 0.0, -half_extent],
             "max": [half_extent, max_height, half_extent]},
            {"bufferView": 1, "componentType": 5126, "count": vertex_count, "type": "VEC3"},
            {"bufferView": 2, "componentType": 5126, "count": vertex_count, "type": "VEC2"},
            {"bufferView": 3, "componentType": 5125, "count": len(indices), "type": "SCALAR"},
        ],
    }
    (output_dir / "kebnekaise_terrain.gltf").write_text(
        json.dumps(gltf, indent=2) + "\n", encoding="utf-8")
    return base_height, spacing, grid


def route_point(mosaic: Image.Image, terrain_grid: Image.Image,
                base_height: float, spacing: float,
                lat: float, lon: float) -> tuple[float, float, float]:
    global_x, global_y = global_pixel(lat, lon)
    local_x = global_x - min(TILE_X) * 256
    local_y = global_y - min(TILE_Y) * 256
    source_pixel_spacing = spacing * (GRID_SIZE - 1) / (mosaic.width - 1)
    x = ((local_x - (mosaic.width - 1) * 0.5) * source_pixel_spacing
         / METERS_PER_WORLD_UNIT)
    z = ((local_y - (mosaic.height - 1) * 0.5) * source_pixel_spacing
         / METERS_PER_WORLD_UNIT)
    grid_x = local_x * (GRID_SIZE - 1) / (mosaic.width - 1)
    grid_y = local_y * (GRID_SIZE - 1) / (mosaic.height - 1)
    y = (bilinear_height(terrain_grid, grid_x, grid_y) - base_height) / METERS_PER_WORLD_UNIT
    return x, y, z


def load_route(path: Path) -> list[tuple[float, float]]:
    data = json.loads(path.read_text(encoding="utf-8"))
    relation = next(element for element in data["elements"]
                    if element["type"] == "relation")
    segments = [
        [(point["lat"], point["lon"]) for point in member.get("geometry", [])]
        for member in relation["members"]
        if member.get("geometry")
    ]
    route = list(reversed(segments[0]))
    for segment in segments[1:]:
        end = route[-1]
        start_distance = math.hypot(segment[0][0] - end[0], segment[0][1] - end[1])
        end_distance = math.hypot(segment[-1][0] - end[0], segment[-1][1] - end[1])
        route.extend(segment if start_distance <= end_distance else reversed(segment))
    return route


def write_route(mosaic: Image.Image, terrain_grid: Image.Image,
                base_height: float, spacing: float, route_path: Path,
                output_dir: Path) -> None:
    route_lat_lon = load_route(route_path)
    points = [route_point(mosaic, terrain_grid, base_height, spacing, lat, lon)
              for lat, lon in route_lat_lon]
    ring_sides = 10
    horizontal_radius = 0.075
    vertical_radius = 0.032
    center_lift = 0.042
    positions = []
    normals = []
    for index, point in enumerate(points):
        previous = points[max(index - 1, 0)]
        following = points[min(index + 1, len(points) - 1)]
        dx, dz = following[0] - previous[0], following[2] - previous[2]
        length = math.hypot(dx, dz) or 1.0
        side_x, side_z = -dz / length, dx / length
        for side in range(ring_sides):
            angle = side * math.tau / ring_sides
            cosine, sine = math.cos(angle), math.sin(angle)
            positions.extend((
                point[0] + side_x * horizontal_radius * cosine,
                point[1] + center_lift + vertical_radius * sine,
                point[2] + side_z * horizontal_radius * cosine,
            ))
            normal_length = math.hypot(cosine, sine) or 1.0
            normals.extend((side_x * cosine / normal_length,
                            sine / normal_length,
                            side_z * cosine / normal_length))
    indices = []
    for point_index in range(len(points) - 1):
        ring = point_index * ring_sides
        next_ring = ring + ring_sides
        for side in range(ring_sides):
            following_side = (side + 1) % ring_sides
            a, b = ring + side, ring + following_side
            c, d = next_ring + side, next_ring + following_side
            indices.extend((a, c, b, b, c, d))

    position_bytes = struct.pack(f"<{len(positions)}f", *positions)
    normal_bytes = struct.pack(f"<{len(normals)}f", *normals)
    index_bytes = struct.pack(f"<{len(indices)}I", *indices)
    offsets = []
    binary = bytearray()
    for chunk in (position_bytes, normal_bytes, index_bytes):
        while len(binary) % 4:
            binary.append(0)
        offsets.append(len(binary))
        binary.extend(chunk)
    (output_dir / "kebnekaise_route.bin").write_bytes(binary)
    vertex_count = len(positions) // 3
    route_gltf = {
        "asset": {"version": "2.0", "generator": "Tiny Hippie route generator"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0, "name": "Vastra leden route"}],
        "meshes": [{"primitives": [{
            "attributes": {"POSITION": 0, "NORMAL": 1},
            "indices": 2,
            "material": 0,
        }]}],
        "materials": [{
            "name": "navigation orange",
            "pbrMetallicRoughness": {
                "baseColorFactor": [1.0, 0.16, 0.015, 1.0],
                "metallicFactor": 0.0,
                "roughnessFactor": 0.38,
            },
        }],
        "buffers": [{"uri": "kebnekaise_route.bin", "byteLength": len(binary)}],
        "bufferViews": [
            {"buffer": 0, "byteOffset": offsets[0], "byteLength": len(position_bytes), "target": 34962},
            {"buffer": 0, "byteOffset": offsets[1], "byteLength": len(normal_bytes), "target": 34962},
            {"buffer": 0, "byteOffset": offsets[2], "byteLength": len(index_bytes), "target": 34963},
        ],
        "accessors": [
            {"bufferView": 0, "componentType": 5126, "count": vertex_count, "type": "VEC3"},
            {"bufferView": 1, "componentType": 5126, "count": vertex_count, "type": "VEC3"},
            {"bufferView": 2, "componentType": 5125, "count": len(indices), "type": "SCALAR"},
        ],
    }
    (output_dir / "kebnekaise_route.gltf").write_text(
        json.dumps(route_gltf, indent=2) + "\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("tile_dir", type=Path)
    parser.add_argument("output_dir", type=Path)
    parser.add_argument(
        "--route-json", type=Path,
        default=Path("Resources/Terrain/Kebnekaise/vastra_leden.osm.json"))
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    encoded_mosaic = stitch_tiles(args.tile_dir)
    elevation_image = decode_elevation_image(encoded_mosaic)
    write_texture(elevation_image, args.output_dir / "kebnekaise_terrain.png")
    base_height, spacing, terrain_grid = write_terrain(
        elevation_image, args.output_dir)
    write_route(elevation_image, terrain_grid, base_height, spacing, args.route_json,
                args.output_dir)

    # The live scene deliberately uses this generic import result. Keeping the
    # older output above makes visual comparisons and regression tests possible.
    gazebo_source = emit_gazebo_source(elevation_image, args.output_dir)
    spec = gazebo_importer.read_spec(gazebo_source)
    imported_heights = gazebo_importer.write_terrain(
        spec, args.output_dir, "kebnekaise_imported", GRID_SIZE,
        METERS_PER_WORLD_UNIT)
    gazebo_importer.write_route(
        gazebo_importer.route_points(
            spec, imported_heights, args.route_json, METERS_PER_WORLD_UNIT),
        args.output_dir, "kebnekaise_imported", METERS_PER_WORLD_UNIT)


if __name__ == "__main__":
    main()
