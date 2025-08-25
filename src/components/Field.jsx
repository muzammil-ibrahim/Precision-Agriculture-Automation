// components/Field.jsx
import React from "react";
import { UncontrolledReactSVGPanZoom } from "react-svg-pan-zoom";
import { polygonPath } from "../utils/geometry";

export default function Field({
  geofence,
  points,
  slotSize,
  drilledPoints,
  view,
  setMousePos,
  setHoveredPoint,
  isDrilling,
  shakeOffset,
  svgRotation,
  FRAME_W,
  FRAME_H,
  tractorTop,
  drillIcon,
}) {
  return (
    <UncontrolledReactSVGPanZoom
      width="100%"
      height="100%"
      tool="auto"
      detectAutoPan={false}
      background="#fff"
      scaleFactor={1.2}
    >
      <svg
        width="100%"
        height="100%"
        style={{ background: "#e5e7eb", borderRadius: "8px" }}
        onMouseMove={(e) => setMousePos({ x: e.clientX, y: e.clientY })}
      >
        <g
          transform={`translate(${FRAME_W / 2 - view.tx * view.scale}, ${
            FRAME_H / 2 - view.ty * view.scale
          }) scale(${view.scale})`}
        >
          <path
            d={polygonPath(geofence)}
            fill="#d1fae5"
            stroke="#065f46"
            strokeWidth={2 / view.scale}
          />
          {points.map((p) => (
            <g
              key={p.id}
              onMouseEnter={() => {
                setHoveredPoint(p);
              }}
              onMouseLeave={() => setHoveredPoint(null)}
              onMouseMove={(e) => {
                setMousePos({ x: e.clientX, y: e.clientY });
              }}
            >
              <rect
                x={p.x - slotSize / 2}
                y={p.y - slotSize / 2}
                width={slotSize}
                height={slotSize}
                fill={drilledPoints.includes(p.id) ? "#a7f3d0" : "#f20808ff"}
                stroke="#374151"
              />
              <circle cx={p.x} cy={p.y} r={6} fill="#2563eb" />
              <text
                x={p.x}
                y={p.y + 3}
                fontSize={6}
                textAnchor="middle"
                fill="#fff"
              >
                {p.id}
              </text>
            </g>
          ))}
        </g>

        <g transform={`rotate(${svgRotation}, 50, 50)`}>
          <image
            href={isDrilling ? drillIcon : tractorTop}
            x={FRAME_W / 2 - 18 + (shakeOffset.x || 0)}
            y={FRAME_H / 2 - 18 + (shakeOffset.y || 0)}
            width="30"
            height="30"
          />
        </g>
      </svg>
    </UncontrolledReactSVGPanZoom>
  );
}
