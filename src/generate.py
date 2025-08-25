import pandas as pd
import matplotlib.pyplot as plt
from pyproj import Transformer
from shapely.geometry import Polygon, Point
from shapely import affinity
import numpy as np
import os

# -----------------------
# Helper: find UTM zone
# -----------------------
def utm_zone_from_lon(lon):
    return int((lon + 180) / 6) + 1


col_spacing_ft = 4  # spacing between points in a row (x direction)
row_spacing_ft = 10  # spacing between rows (y direction)
border_margin_ft = 4  # empty space near edges

# Convert feet → meters
ft_to_m = 0.3048
col_spacing_m = col_spacing_ft * ft_to_m
row_spacing_m = row_spacing_ft * ft_to_m
border_margin_m = border_margin_ft * ft_to_m

# -----------------------
# File paths
# -----------------------
base_dir = r"C:\Users\Muzammil\OneDrive\Desktop\my-app"
geofence_input_path = os.path.join(base_dir, "src", "geofence.csv")  # <-- INPUT remains same
geofence_output_path = os.path.join(base_dir, "public", "geofence_converted.csv")
points_output_path = os.path.join(base_dir, "public", "points_converted.csv")

# -----------------------
# Load geofence CSV
# -----------------------
df_geofence = pd.read_csv(geofence_input_path)  # latitude, longitude

# -----------------------
# Detect UTM zone from geofence center
# -----------------------
mean_lon = df_geofence["longitude"].mean()
zone = utm_zone_from_lon(mean_lon)

if df_geofence["latitude"].mean() >= 0:
    epsg_code = 32600 + zone
else:
    epsg_code = 32700 + zone

print(f"Detected UTM zone: {zone}, EPSG:{epsg_code}")

# -----------------------
# Transformer WGS84 → UTM
# -----------------------
transformer = Transformer.from_crs("EPSG:4326", f"EPSG:{epsg_code}", always_xy=True)

# -----------------------
# Pick reference point
# -----------------------
ref_lat = df_geofence["latitude"].min()
ref_lon = df_geofence["longitude"].min()
ref_x, ref_y = transformer.transform(ref_lon, ref_lat)

# -----------------------
# Convert geofence to UTM
# -----------------------
df_geofence["x_m"], df_geofence["y_m"] = transformer.transform(
    df_geofence["longitude"].values,
    df_geofence["latitude"].values
)
df_geofence["x"] = (df_geofence["x_m"] - ref_x) * 100
df_geofence["y"] = (df_geofence["y_m"] - ref_y) * 100

# Save geofence in cm
df_geofence[["x", "y"]].to_csv(geofence_output_path, index=False)

# -----------------------
# Create polygon in meters
# -----------------------
poly = Polygon(zip(df_geofence["x_m"], df_geofence["y_m"]))

# -----------------------
# Shrink polygon by border margin
# -----------------------
poly_shrunk = poly.buffer(-border_margin_m)

# -----------------------
# Generate grid points inside polygon
# -----------------------
min_x, min_y, max_x, max_y = poly_shrunk.bounds
points_x = np.arange(min_x, max_x, col_spacing_m)
points_y = np.arange(min_y, max_y, row_spacing_m)

grid_points = []
for y in points_y:
    for x in points_x:
        p = Point(x, y)
        if poly_shrunk.contains(p):
            x_cm = (x - ref_x) * 100
            y_cm = (y - ref_y) * 100
            grid_points.append((x_cm, y_cm))

# -----------------------
# Save points
# -----------------------
df_points = pd.DataFrame(grid_points, columns=["x", "y"])
df_points.to_csv(points_output_path, index=False)

print(f"✅ Generated {len(df_points)} points.")
