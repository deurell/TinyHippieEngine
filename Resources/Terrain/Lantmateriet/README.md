# Lantmäteriet terrain pipeline

`scripts/generate_lantmateriet_terrain.py` queries Lantmäteriet's STAC
catalogs, streams the selected elevation and orthophoto COG windows, creates a
Gazebo-compatible source package, and converts it to indexed glTF for Tiny
Hippie Engine.

Order free access to **Markhöjdmodell Nedladdning** and **Ortofoto
Nedladdning** in [Geotorget](https://geotorget.lantmateriet.se/). Catalog
search is public, but the COG download host returns HTTP 401 until those two
permissions have been granted. Credentials are read from the environment and
are not stored:

```bash
export LANTMATERIET_USERNAME='...'
export LANTMATERIET_PASSWORD='...'

uv run --with rasterio --with pyproj --with numpy --with pillow \
  scripts/generate_lantmateriet_terrain.py \
  --bounds 18.42 67.84 18.62 67.96 \
  --name kebnekaise_lantmateriet \
  --output Resources/Terrain/Lantmateriet/Kebnekaise
```

Catalog discovery does not require credentials and can be tested with
`--discover-only`. Output includes source item identifiers and catalog URLs for
attribution and reproducibility. The current STAC catalogs declare both source
products as CC BY 4.0; retain attribution to Lantmäteriet with published work.
