geofence_coords = [
    (12.971598, 77.594566),
    (12.971700, 77.594600),
    (12.971650, 77.594700),
    (12.971550, 77.594650),
    (12.971598, 77.594566)  # Closing the polygon by repeating first point
]
import matplotlib.pyplot as plt

# Separate lats and lons
lats, lons = zip(*geofence_coords)

plt.figure(figsize=(6,6))
plt.plot(lons, lats, 'b-', linewidth=2)  # Polygon outline
plt.fill(lons, lats, 'skyblue', alpha=0.4)  # Fill with transparency

plt.xlabel("Longitude")
plt.ylabel("Latitude")
plt.title("Geofence Polygon")
plt.grid(True)
plt.axis('equal')  # Equal scaling for lat/lon

plt.show()
