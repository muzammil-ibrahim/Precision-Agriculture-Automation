// components/StatusPanel.jsx
import React from "react";

export default function StatusPanel({
  points,
  currentIndex,
  isDrilling,
  running,
  colSpacing,
  rowSpacing,
  borderMargin,
  setColSpacing,
  setRowSpacing,
  setBorderMargin,
  handleGenerate,
  handleClear,
  onStartPause, // passed from App
}) {
  return (
    <div
      style={{
        minWidth: "350px",
        background: "#fff",
        padding: "1rem",
        borderRadius: "8px",
        boxShadow: "0 2px 8px rgba(0,0,0,0.1)",
        color: "#000",
        flexShrink: 0,
        overflowY: "auto",
      }}
    >
      <h3>Status Panel</h3>
      {points.length === 0 ? (
        <p>Loading data...</p>
      ) : (
        <>
          <p>
            <b>Current Point:</b>{" "}
            {currentIndex >= 0 ? points[currentIndex]?.id : "-"}
          </p>
          <p>
            <b>Status:</b>{" "}
            {isDrilling ? "Drilling" : running ? "Moving" : "Idle"}
          </p>
          <p>
            <b>Progress:</b>{" "}
            {currentIndex >= 0 ? currentIndex + 1 : 0} / {points.length}
          </p>
        </>
      )}

      <div>
        <label>
          Column Spacing (ft):
          <input
            type="number"
            value={colSpacing}
            onChange={(e) => setColSpacing(e.target.value)}
            style={{
              backgroundColor: "white",
              color: "black",
              border: "1px solid #ccc",
              padding: "8px",
              borderRadius: "4px",
            }}
          />
        </label>
        <br />
        <label>
          Row Spacing (ft):
          <input
            type="number"
            value={rowSpacing}
            onChange={(e) => setRowSpacing(e.target.value)}
            style={{
              backgroundColor: "white",
              color: "black",
              border: "1px solid #ccc",
              padding: "8px",
              borderRadius: "4px",
            }}
          />
        </label>
        <br />
        <label>
          Border Margin (ft):
          <input
            type="number"
            value={borderMargin}
            onChange={(e) => setBorderMargin(e.target.value)}
            style={{
              backgroundColor: "white",
              color: "black",
              border: "1px solid #ccc",
              padding: "8px",
              borderRadius: "4px",
            }}
          />
        </label>
        <br />
        <button onClick={handleGenerate}>Generate</button> <br />
        <br />
        <button onClick={handleClear}>Clear Data</button> <br />
        <br />
      </div>

      <button onClick={onStartPause}>
        {running ? "Pause" : "Start"}
      </button>
      <br />
    </div>
  );
}
