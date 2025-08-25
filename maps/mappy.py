import folium

# Example: Hyderabad, India
location_coords = [17.400614, 78.396611]  # Latitude, Longitude

# Create map centered on Hyderabad
m = folium.Map(location=location_coords, zoom_start=18)  # zoom_start controls how close

# Optional: Add a marker
folium.Marker(
    location=location_coords,
    popup="Hyderabad, India",
    icon=folium.Icon(color="blue", icon="info-sign")
).add_to(m)

# Save to HTML file
m.save("hyderabad_map.html")
