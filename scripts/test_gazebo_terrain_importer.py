#!/usr/bin/env python3
"""Deterministic smoke test for the Gazebo terrain importer."""

import json
import subprocess
import sys
import tempfile
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
IMPORTER = ROOT / "scripts" / "import_gazebo_terrain.py"


def main() -> int:
    with tempfile.TemporaryDirectory() as temporary:
        source = Path(temporary) / "sample"
        output = Path(temporary) / "output"
        (source / "mesh").mkdir(parents=True)
        height = Image.new("I;16", (3, 3))
        height.putdata([0, 16384, 32768, 8192, 32768, 49152, 16384, 49152, 65535])
        height.save(source / "mesh" / "height_map.png")
        Image.new("RGB", (4, 4), (20, 80, 30)).save(source / "mesh" / "aerial.png")
        (source / "sample.world").write_text("""<sdf version="1.9"><world name="sample">
          <spherical_coordinates><latitude_deg>67.9</latitude_deg><longitude_deg>18.5</longitude_deg></spherical_coordinates>
          <model name="terrain"><link name="ground"><visual name="visual"><geometry><heightmap>
          <texture><diffuse>mesh/aerial.png</diffuse></texture><uri>mesh/height_map.png</uri>
          <size>400 200 100</size><pos>0 0 0</pos></heightmap></geometry></visual></link></model>
        </world></sdf>""")
        (source / "route.geojson").write_text(json.dumps({"type": "LineString", "coordinates": [
            [18.4995, 67.8998], [18.5, 67.9], [18.5005, 67.9002]]}))
        result = subprocess.run([sys.executable, str(IMPORTER), str(source), str(output),
                                 "--name", "sample", "--grid-size", "3",
                                 "--metres-per-unit", "100", "--route", str(source / "route.geojson")],
                                text=True, capture_output=True, check=False)
        if result.returncode:
            raise AssertionError(result.stderr)
        terrain = json.loads((output / "sample_terrain.gltf").read_text())
        route = json.loads((output / "sample_route.gltf").read_text())
        manifest = json.loads((output / "sample.terrain.json").read_text())
        assert terrain["accessors"][0]["count"] == 9
        assert terrain["accessors"][3]["count"] == 24
        assert terrain["accessors"][0]["max"] == [2.0, 1.0, 1.0]
        assert route["accessors"][0]["count"] == 30
        assert manifest["sizeMetres"] == [400.0, 200.0, 100.0]
        assert (output / "sample_aerial.png").is_file()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
