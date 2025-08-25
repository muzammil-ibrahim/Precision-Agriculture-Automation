// utils/geometry.js
export function bbox(points) {
  const xs = points.map((p) => p.x);
  const ys = points.map((p) => p.y);
  return {
    minX: Math.min(...xs),
    maxX: Math.max(...xs),
    minY: Math.min(...ys),
    maxY: Math.max(...ys),
    width: Math.max(...xs) - Math.min(...xs),
    height: Math.max(...ys) - Math.min(...ys),
  };
}

export function lerp(a, b, t) {
  return a + (b - a) * t;
}

export function polygonPath(pointsArr) {
  if (!pointsArr.length) return "";
  return (
    pointsArr.map((p, i) => `${i === 0 ? "M" : "L"}${p.x},${p.y}`).join(" ") + " Z"
  );
}
