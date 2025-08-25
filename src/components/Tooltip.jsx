// components/Tooltip.jsx
import React from "react";

export default function Tooltip({ hoveredPoint, mousePos }) {
  if (!hoveredPoint) return null;
  return (
    <div
      style={{
        color: "black",
        position: "fixed",
        left: mousePos.x + 10,
        top: mousePos.y + 10,
        background: "white",
        border: "1px solid black",
        padding: "4px",
        fontSize: "20px",
        pointerEvents: "none",
      }}
    >
      ID: {hoveredPoint.id}
      <br />
      Plant Type: Papaya
    </div>
  );
}
