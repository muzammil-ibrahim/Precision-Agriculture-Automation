// utils/zigzag.js
function getColumnCount(points) {
  const uniqueXs = [...new Set(points.map(p => p.x))].sort((a, b) => a - b);
  return uniqueXs.length;
}

export function applyZigzagOrder(list) {
  if (list.length <= 3) return list;
  const colCount = getColumnCount(list);
  const rows = [];
  for (let i = 0; i < list.length; i += colCount) {
    rows.push(list.slice(i, i + colCount));
  }
  for (let i = 1; i < rows.length; i += 2) {
    rows[i].reverse();
  }
  return rows.flat();
}
