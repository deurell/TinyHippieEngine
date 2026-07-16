#!/usr/bin/env python3
"""Validate Tiny Hippie Engine scene JSON files."""

from __future__ import annotations

import argparse
import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any


NODE_TYPES = {
    "SceneNode",
    "CameraNode",
    "LightNode",
    "Light2DNode",
    "MeshNode",
    "SpriteNode",
    "SpriteAnimationNode",
    "SpriteBatchNode",
    "FogOverlayNode",
    "TextNode",
    "TileMapNode",
    "PlaneNode",
    "PhongShapeNode",
    "ParticleSystemNode",
}

ROOT_FIELDS = {"name", "nodes"}
NODE_FIELDS = {
    "name",
    "type",
    "renderLayer",
    "transform",
    "children",
    "mesh",
    "image",
    "sourceRect",
    "text",
    "alignment",
    "anchor",
    "fontSize",
    "textColor",
    "shadowColor",
    "shadowOffset",
    "plane",
    "shape",
    "particle",
    "billboard",
    "flipX",
    "flipY",
    "flipDiagonal",
    "active",
    "fov",
    "projection",
    "orthographicHeight",
    "lookAt",
    "color",
    "visualizer",
    "material",
    "animation",
    "light",
    "light2D",
    "fogOverlay",
    "tileMap",
    "spriteBatch",
    "spriteAnimation",
}

TRANSFORM_FIELDS = {"position", "rotationEuler", "scale"}
VISUALIZER_FIELDS = {
    "lightDirection",
    "lightColor",
    "ambientStrength",
    "specularStrength",
    "shininess",
}
MATERIAL_FIELDS = {"diffuse", "ambient", "specular", "shininess"}
ANIMATION_FIELDS = {"clip", "playing", "looping", "playbackSpeed"}
LIGHT_FIELDS = {
    "kind",
    "direction",
    "color",
    "intensity",
    "ambientStrength",
    "active",
}
LIGHT2D_FIELDS = {
    "color",
    "radius",
    "intensity",
    "softness",
    "flickerAmount",
    "flickerSpeed",
}
FOG_OVERLAY_FIELDS = {
    "image",
    "color",
    "tiling",
    "scrollSpeed",
    "alpha",
    "softness",
    "secondLayerStrength",
    "secondLayerScrollSpeed",
    "pulseAmount",
    "pulseSpeed",
}
TILE_MAP_FIELDS = {
    "image",
    "firstGid",
    "mapWidth",
    "mapHeight",
    "tileWidth",
    "tileHeight",
    "columns",
    "tileWorldSize",
    "layers",
}
TILE_LAYER_FIELDS = {"name", "z", "data"}
SPRITE_BATCH_FIELDS = {"image", "sprites"}
SPRITE_BATCH_ITEM_FIELDS = {
    "position",
    "size",
    "rotationDegrees",
    "sourceRect",
    "flipX",
    "flipY",
    "flipDiagonal",
}
SPRITE_ANIMATION_FIELDS = {"fps", "playing", "looping", "frames"}

ALIGNMENTS = {"Left", "Center", "Right"}
ANCHORS = {
    "TopLeft",
    "TopCenter",
    "TopRight",
    "CenterLeft",
    "Center",
    "CenterRight",
    "BottomLeft",
    "BottomCenter",
    "BottomRight",
}
PROJECTIONS = {"Perspective", "Orthographic"}
LIGHT_KINDS = {"Directional"}
PLANE_TYPES = {"Simple", "Spinner"}
SHAPE_TYPES = {"Cube", "Sphere", "Cylinder"}
PARTICLE_PRESETS = {"Default", "SoftGlowBurst", "WaterFountain"}

FLIP_MASK = 0x80000000 | 0x40000000 | 0x20000000


@dataclass
class Diagnostic:
    level: str
    path: str
    message: str


class Validator:
    def __init__(self, scene_path: Path, repo_root: Path) -> None:
        self.scene_path = scene_path
        self.repo_root = repo_root
        self.diagnostics: list[Diagnostic] = []

    def error(self, path: str, message: str) -> None:
        self.diagnostics.append(Diagnostic("error", path, message))

    def warn(self, path: str, message: str) -> None:
        self.diagnostics.append(Diagnostic("warning", path, message))

    def validate(self) -> None:
        try:
            with self.scene_path.open("r", encoding="utf-8") as handle:
                scene = json.load(handle)
        except json.JSONDecodeError as exc:
            self.error("$", f"invalid JSON at line {exc.lineno}, column {exc.colno}: {exc.msg}")
            return
        except OSError as exc:
            self.error("$", f"failed to open scene: {exc}")
            return

        if not isinstance(scene, dict):
            self.error("$", "scene document must be an object")
            return

        self.check_unknown("$", scene, ROOT_FIELDS)
        self.check_string(scene, "name", "$.name", required=False)
        nodes = scene.get("nodes")
        if not isinstance(nodes, list):
            self.error("$.nodes", "scene document must contain a nodes array")
            return
        for index, node in enumerate(nodes):
            self.validate_node(node, f"$.nodes[{index}]")

    def validate_node(self, node: Any, path: str) -> None:
        if not isinstance(node, dict):
            self.error(path, "node must be an object")
            return

        self.check_unknown(path, node, NODE_FIELDS)
        self.check_string(node, "name", f"{path}.name", required=False)
        node_type = node.get("type", "SceneNode")
        if not isinstance(node_type, str):
            self.error(f"{path}.type", "type must be a string")
            node_type = "SceneNode"
        elif node_type not in NODE_TYPES:
            self.error(f"{path}.type", f"unknown node type {node_type!r}")

        self.validate_common_fields(node, path)
        self.validate_payload_for_type(node, node_type, path)

        children = node.get("children")
        if children is not None:
            if not isinstance(children, list):
                self.error(f"{path}.children", "children must be an array")
            else:
                for index, child in enumerate(children):
                    self.validate_node(child, f"{path}.children[{index}]")

    def validate_common_fields(self, node: dict[str, Any], path: str) -> None:
        if "transform" in node:
            transform = self.check_object(node, "transform", f"{path}.transform")
            if transform is not None:
                self.check_unknown(f"{path}.transform", transform, TRANSFORM_FIELDS)
                position = self.check_vec(transform, "position", 3, f"{path}.transform.position")
                scale = self.check_vec(transform, "scale", 3, f"{path}.transform.scale")
                self.check_vec(transform, "rotationEuler", 3, f"{path}.transform.rotationEuler")
                if position and max(abs(value) for value in position) > 1000.0:
                    self.warn(f"{path}.transform.position", "position is very far from the scene origin")
                if scale:
                    if any(value == 0.0 for value in scale):
                        self.warn(f"{path}.transform.scale", "zero scale can make the node invisible")
                    if max(abs(value) for value in scale) > 100.0:
                        self.warn(f"{path}.transform.scale", "scale is very large for meter-based scene units")

        self.check_vec(node, "sourceRect", 4, f"{path}.sourceRect")
        if "sourceRect" in node:
            self.check_source_rect(node["sourceRect"], f"{path}.sourceRect")
        self.check_vec(node, "lookAt", 3, f"{path}.lookAt")
        self.check_vec(node, "color", 4, f"{path}.color")
        self.check_vec(node, "textColor", 4, f"{path}.textColor")
        self.check_vec(node, "shadowColor", 4, f"{path}.shadowColor")
        self.check_vec(node, "shadowOffset", 2, f"{path}.shadowOffset")

        self.check_string(node, "mesh", f"{path}.mesh", required=False)
        self.check_string(node, "image", f"{path}.image", required=False)
        self.check_string(node, "text", f"{path}.text", required=False)
        self.check_int(node, "renderLayer", f"{path}.renderLayer")
        self.check_number(node, "fontSize", f"{path}.fontSize")
        self.check_number(node, "fov", f"{path}.fov")
        self.check_number(node, "orthographicHeight", f"{path}.orthographicHeight")
        for key in ("billboard", "flipX", "flipY", "flipDiagonal", "active"):
            self.check_bool(node, key, f"{path}.{key}")

        self.check_enum(node, "alignment", ALIGNMENTS, f"{path}.alignment")
        self.check_enum(node, "anchor", ANCHORS, f"{path}.anchor")
        self.check_enum(node, "projection", PROJECTIONS, f"{path}.projection")
        self.check_enum(node, "plane", PLANE_TYPES, f"{path}.plane")
        self.check_enum(node, "shape", SHAPE_TYPES, f"{path}.shape")
        self.check_enum(node, "particle", PARTICLE_PRESETS, f"{path}.particle")

        self.validate_named_object(node, "visualizer", VISUALIZER_FIELDS, path)
        self.validate_named_object(node, "material", MATERIAL_FIELDS, path)
        self.validate_named_object(node, "animation", ANIMATION_FIELDS, path)
        self.validate_light(node, path)
        self.validate_light2d(node, path)
        self.validate_fog_overlay(node, path)
        self.validate_tile_map(node, path)
        self.validate_sprite_batch(node, path)
        self.validate_sprite_animation(node, path)

    def validate_payload_for_type(self, node: dict[str, Any], node_type: str, path: str) -> None:
        if node_type == "MeshNode":
            self.require_asset_string(node, "mesh", f"{path}.mesh")
        elif node_type == "SpriteNode":
            self.require_asset_string(node, "image", f"{path}.image")
        elif node_type == "SpriteAnimationNode":
            self.require_asset_string(node, "image", f"{path}.image")
            if "spriteAnimation" not in node:
                self.error(f"{path}.spriteAnimation", "SpriteAnimationNode requires spriteAnimation")
        elif node_type == "SpriteBatchNode":
            if "spriteBatch" not in node:
                self.error(f"{path}.spriteBatch", "SpriteBatchNode requires spriteBatch")
        elif node_type == "FogOverlayNode":
            if "fogOverlay" not in node:
                self.error(f"{path}.fogOverlay", "FogOverlayNode requires fogOverlay")
        elif node_type == "TextNode":
            self.check_string(node, "text", f"{path}.text", required=True)
        elif node_type == "TileMapNode":
            if "tileMap" not in node:
                self.error(f"{path}.tileMap", "TileMapNode requires tileMap")
        elif node_type == "LightNode":
            if "light" not in node:
                self.error(f"{path}.light", "LightNode requires light")
        elif node_type == "Light2DNode":
            if "light2D" not in node:
                self.error(f"{path}.light2D", "Light2DNode requires light2D")

    def validate_named_object(
        self, node: dict[str, Any], key: str, allowed: set[str], path: str
    ) -> dict[str, Any] | None:
        obj = self.check_object(node, key, f"{path}.{key}")
        if obj is not None:
            self.check_unknown(f"{path}.{key}", obj, allowed)
        return obj

    def validate_light(self, node: dict[str, Any], path: str) -> None:
        light = self.validate_named_object(node, "light", LIGHT_FIELDS, path)
        if light is None:
            return
        self.check_enum(light, "kind", LIGHT_KINDS, f"{path}.light.kind")
        self.check_vec(light, "direction", 3, f"{path}.light.direction")
        self.check_vec(light, "color", 3, f"{path}.light.color")
        self.check_number(light, "intensity", f"{path}.light.intensity")
        self.check_number(light, "ambientStrength", f"{path}.light.ambientStrength")
        self.check_bool(light, "active", f"{path}.light.active")

    def validate_light2d(self, node: dict[str, Any], path: str) -> None:
        light = self.validate_named_object(node, "light2D", LIGHT2D_FIELDS, path)
        if light is None:
            return
        self.check_vec(light, "color", 4, f"{path}.light2D.color")
        for key in ("radius", "intensity", "softness", "flickerAmount", "flickerSpeed"):
            self.check_number(light, key, f"{path}.light2D.{key}")

    def validate_fog_overlay(self, node: dict[str, Any], path: str) -> None:
        fog = self.validate_named_object(node, "fogOverlay", FOG_OVERLAY_FIELDS, path)
        if fog is None:
            return
        self.require_asset_string(fog, "image", f"{path}.fogOverlay.image")
        self.check_vec(fog, "color", 4, f"{path}.fogOverlay.color")
        self.check_vec(fog, "tiling", 2, f"{path}.fogOverlay.tiling")
        self.check_vec(fog, "scrollSpeed", 2, f"{path}.fogOverlay.scrollSpeed")
        self.check_vec(
            fog,
            "secondLayerScrollSpeed",
            2,
            f"{path}.fogOverlay.secondLayerScrollSpeed",
        )
        for key in ("alpha", "softness", "secondLayerStrength", "pulseAmount", "pulseSpeed"):
            self.check_number(fog, key, f"{path}.fogOverlay.{key}")

    def validate_tile_map(self, node: dict[str, Any], path: str) -> None:
        tile_map = self.validate_named_object(node, "tileMap", TILE_MAP_FIELDS, path)
        if tile_map is None:
            return
        self.require_asset_string(tile_map, "image", f"{path}.tileMap.image")
        first_gid = self.check_int(tile_map, "firstGid", f"{path}.tileMap.firstGid", required=True)
        map_width = self.check_int(tile_map, "mapWidth", f"{path}.tileMap.mapWidth", required=True)
        map_height = self.check_int(tile_map, "mapHeight", f"{path}.tileMap.mapHeight", required=True)
        self.check_int(tile_map, "tileWidth", f"{path}.tileMap.tileWidth", required=True)
        self.check_int(tile_map, "tileHeight", f"{path}.tileMap.tileHeight", required=True)
        self.check_int(tile_map, "columns", f"{path}.tileMap.columns", required=True)
        self.check_number(tile_map, "tileWorldSize", f"{path}.tileMap.tileWorldSize", required=True)
        if first_gid is not None and first_gid <= 0:
            self.error(f"{path}.tileMap.firstGid", "firstGid must be greater than zero")

        layers = tile_map.get("layers")
        if layers is None:
            return
        if not isinstance(layers, list):
            self.error(f"{path}.tileMap.layers", "layers must be an array")
            return
        expected_count = None
        if map_width is not None and map_height is not None:
            expected_count = map_width * map_height
        for index, layer in enumerate(layers):
            layer_path = f"{path}.tileMap.layers[{index}]"
            if not isinstance(layer, dict):
                self.error(layer_path, "tile layer must be an object")
                continue
            self.check_unknown(layer_path, layer, TILE_LAYER_FIELDS)
            self.check_string(layer, "name", f"{layer_path}.name", required=False)
            self.check_number(layer, "z", f"{layer_path}.z")
            data = layer.get("data")
            if data is None:
                continue
            if not isinstance(data, list):
                self.error(f"{layer_path}.data", "tile layer data must be an array")
                continue
            if expected_count is not None and len(data) != expected_count:
                self.error(
                    f"{layer_path}.data",
                    f"tile layer data has {len(data)} values, expected {expected_count}",
                )
            for tile_index, value in enumerate(data):
                if not isinstance(value, int) or value < 0:
                    self.error(f"{layer_path}.data[{tile_index}]", "tile gid must be a non-negative integer")
                    continue
                gid = value & ~FLIP_MASK
                if gid != 0 and first_gid is not None and gid < first_gid:
                    self.error(f"{layer_path}.data[{tile_index}]", "tile gid is below firstGid")

    def validate_sprite_batch(self, node: dict[str, Any], path: str) -> None:
        batch = self.validate_named_object(node, "spriteBatch", SPRITE_BATCH_FIELDS, path)
        if batch is None:
            return
        self.require_asset_string(batch, "image", f"{path}.spriteBatch.image")
        sprites = batch.get("sprites")
        if sprites is None:
            return
        if not isinstance(sprites, list):
            self.error(f"{path}.spriteBatch.sprites", "sprites must be an array")
            return
        for index, sprite in enumerate(sprites):
            sprite_path = f"{path}.spriteBatch.sprites[{index}]"
            if not isinstance(sprite, dict):
                self.error(sprite_path, "sprite batch item must be an object")
                continue
            self.check_unknown(sprite_path, sprite, SPRITE_BATCH_ITEM_FIELDS)
            self.check_vec(sprite, "position", 3, f"{sprite_path}.position")
            self.check_vec(sprite, "size", 2, f"{sprite_path}.size")
            self.check_number(sprite, "rotationDegrees", f"{sprite_path}.rotationDegrees")
            self.check_vec(sprite, "sourceRect", 4, f"{sprite_path}.sourceRect")
            if "sourceRect" in sprite:
                self.check_source_rect(sprite["sourceRect"], f"{sprite_path}.sourceRect")
            for key in ("flipX", "flipY", "flipDiagonal"):
                self.check_bool(sprite, key, f"{sprite_path}.{key}")

    def validate_sprite_animation(self, node: dict[str, Any], path: str) -> None:
        animation = self.validate_named_object(
            node, "spriteAnimation", SPRITE_ANIMATION_FIELDS, path
        )
        if animation is None:
            return
        self.check_number(animation, "fps", f"{path}.spriteAnimation.fps")
        self.check_bool(animation, "playing", f"{path}.spriteAnimation.playing")
        self.check_bool(animation, "looping", f"{path}.spriteAnimation.looping")
        frames = animation.get("frames")
        if frames is None:
            return
        if not isinstance(frames, list):
            self.error(f"{path}.spriteAnimation.frames", "frames must be an array")
            return
        for index, frame in enumerate(frames):
            self.check_vec_value(frame, 4, f"{path}.spriteAnimation.frames[{index}]")
            self.check_source_rect(frame, f"{path}.spriteAnimation.frames[{index}]")

    def check_unknown(self, path: str, obj: dict[str, Any], allowed: set[str]) -> None:
        for key in sorted(obj.keys() - allowed):
            self.error(f"{path}.{key}", "unknown field")

    def check_object(self, obj: dict[str, Any], key: str, path: str) -> dict[str, Any] | None:
        if key not in obj:
            return None
        value = obj[key]
        if not isinstance(value, dict):
            self.error(path, "must be an object")
            return None
        return value

    def check_string(
        self, obj: dict[str, Any], key: str, path: str, *, required: bool
    ) -> str | None:
        if key not in obj:
            if required:
                self.error(path, "missing required string")
            return None
        value = obj[key]
        if not isinstance(value, str):
            self.error(path, "must be a string")
            return None
        if required and value == "":
            self.error(path, "must not be empty")
        return value

    def require_asset_string(self, obj: dict[str, Any], key: str, path: str) -> str | None:
        value = self.check_string(obj, key, path, required=True)
        if value:
            self.check_asset(value, path)
        return value

    def check_asset(self, value: str, path: str) -> None:
        asset_path = Path(value)
        if not asset_path.is_absolute():
            asset_path = self.repo_root / asset_path
        if not asset_path.exists():
            self.error(path, f"asset does not exist: {value}")

    def check_bool(self, obj: dict[str, Any], key: str, path: str) -> None:
        if key in obj and not isinstance(obj[key], bool):
            self.error(path, "must be a bool")

    def check_number(
        self, obj: dict[str, Any], key: str, path: str, *, required: bool = False
    ) -> float | None:
        if key not in obj:
            if required:
                self.error(path, "missing required number")
            return None
        value = obj[key]
        if not isinstance(value, (int, float)) or isinstance(value, bool):
            self.error(path, "must be a number")
            return None
        return float(value)

    def check_int(
        self, obj: dict[str, Any], key: str, path: str, *, required: bool = False
    ) -> int | None:
        if key not in obj:
            if required:
                self.error(path, "missing required integer")
            return None
        value = obj[key]
        if not isinstance(value, int) or isinstance(value, bool):
            self.error(path, "must be an integer")
            return None
        return value

    def check_enum(
        self, obj: dict[str, Any], key: str, allowed: set[str], path: str
    ) -> None:
        if key not in obj:
            return
        value = obj[key]
        if not isinstance(value, str):
            self.error(path, "must be a string")
        elif value not in allowed:
            options = ", ".join(sorted(allowed))
            self.error(path, f"unknown value {value!r}; expected one of: {options}")

    def check_vec(
        self, obj: dict[str, Any], key: str, size: int, path: str
    ) -> list[float] | None:
        if key not in obj:
            return None
        return self.check_vec_value(obj[key], size, path)

    def check_vec_value(self, value: Any, size: int, path: str) -> list[float] | None:
        if not isinstance(value, list) or len(value) != size:
            self.error(path, f"must be an array with {size} numbers")
            return None
        result: list[float] = []
        for index, item in enumerate(value):
            if not isinstance(item, (int, float)) or isinstance(item, bool):
                self.error(f"{path}[{index}]", "must be a number")
            else:
                result.append(float(item))
        return result if len(result) == size else None

    def check_source_rect(self, value: Any, path: str) -> None:
        rect = self.check_vec_value(value, 4, path)
        if rect is None:
            return
        if rect[2] <= 0.0 or rect[3] <= 0.0:
            self.error(path, "sourceRect width and height must be positive")


def validate_file(path: Path, repo_root: Path, warnings_as_errors: bool) -> int:
    validator = Validator(path, repo_root)
    validator.validate()
    errors = [diag for diag in validator.diagnostics if diag.level == "error"]
    warnings = [diag for diag in validator.diagnostics if diag.level == "warning"]
    for diag in validator.diagnostics:
        print(f"{path}: {diag.level}: {diag.path}: {diag.message}", file=sys.stderr)
    if warnings_as_errors and warnings:
        return 1
    return 1 if errors else 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("scenes", nargs="+", help="Scene JSON files to validate")
    parser.add_argument(
        "--repo-root",
        default=".",
        help="Repository root used to resolve resource-relative asset paths",
    )
    parser.add_argument(
        "--warnings-as-errors",
        action="store_true",
        help="Treat advisory warnings as validation failures",
    )
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    failed = 0
    for scene in args.scenes:
        failed |= validate_file(Path(scene), repo_root, args.warnings_as_errors)
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
