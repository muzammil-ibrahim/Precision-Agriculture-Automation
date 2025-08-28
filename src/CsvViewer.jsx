import React, { useState, useEffect } from "react";
import Papa from "papaparse";

function CsvViewer() {
  const [csvData, setCsvData] = useState([]);
  const [csvHeaders, setCsvHeaders] = useState([]);
  const [csvLoading, setCsvLoading] = useState(false);
  const [csvError, setCsvError] = useState("");

  // Fetch CSV data from backend
  const fetchAndParseCsv = () => {
    setCsvLoading(true);
    fetch("/get_csv2")
      .then(res => {
        if (!res.ok) throw new Error(`Status: ${res.status}`);
        return res.text();
      })
      .then(text => {
        Papa.parse(text, {
          header: true,
          skipEmptyLines: true,
          complete: (res) => {
            setCsvData(res.data);
            setCsvHeaders(res.data.length > 0 ? Object.keys(res.data[0]) : []);
            setCsvLoading(false);
          },
          error: (err) => {
            setCsvError(err.message);
            setCsvLoading(false);
          }
        });
      })
      .catch(err => {
        setCsvError(err.message);
        setCsvLoading(false); 
      });
  };

  useEffect(() => {
    fetchAndParseCsv();
  }, []);

  // Start logging
  const handleStart = async () => {
    const res = await fetch("/start", { method: "POST" });
    const data = await res.json();
    console.log(data);
    alert("Logging started!");
  };

  // Stop logging
  const handleStop = async () => {
    const res = await fetch("/stop", { method: "POST" });
    const data = await res.json();
    console.log(data);
    alert("Logging stopped!");
  };

  return (
    <div style={{ padding: "20px" }}>
      <h2>CSV Data</h2>

      {/* Control buttons */}
      <div style={{ marginBottom: "10px" }}>
        <button onClick={handleStart}>Start</button>
        <button onClick={handleStop}>Stop</button>
        <button onClick={fetchAndParseCsv}>Refresh</button>
      </div>

      <div style={{ display: "flex", gap: "20px", alignItems: "flex-start" }}>
        {/* CSV Table Section (scrollable inside fixed height) */}
        <div style={{ flex: 2, maxHeight: "500px", overflowY: "auto", border: "1px solid #ccc" }}>
          {csvLoading && <p>Loading...</p>}
          {csvError && <p style={{ color: "red" }}>Error: {csvError}</p>}
          {!csvLoading && !csvError && csvData.length > 0 ? (
            <table
              border="1"
              cellPadding="4"
              style={{ borderCollapse: "collapse", width: "100%" }}
            >
              <thead>
                <tr>{csvHeaders.map((h) => <th key={h}>{h}</th>)}</tr>
              </thead>
              <tbody>
                {csvData.map((row, idx) => (
                  <tr key={idx}>
                    {csvHeaders.map((h) => (
                      <td key={h}>{row[h]}</td>
                    ))}
                  </tr>
                ))}
              </tbody>
            </table>
          ) : (
            <p style={{ color: "gray" }}>No CSV data available</p>
          )}
        </div>

        {/* GPS Fix Type Table (fixed, same height as CSV section) */}
        <div style={{ flex: 1, maxHeight: "500px", overflow: "hidden", border: "1px solid #ccc" }}>
          <h3 style={{ margin: "0", padding: "8px", background: "#f9f9f9" }}>GPS Fix Types</h3>
          <table
            border="1"
            cellPadding="4"
            style={{ borderCollapse: "collapse", width: "100%" }}
          >
            <thead>
              <tr>
                <th>Value</th>
                <th>Name</th>
                <th>Description</th>
              </tr>
            </thead>
            <tbody>
              <tr><td>0</td><td>GPS_FIX_TYPE_NO_GPS</td><td>No GPS connected</td></tr>
              <tr><td>1</td><td>GPS_FIX_TYPE_NO_FIX</td><td>No position info, GPS connected</td></tr>
              <tr><td>2</td><td>GPS_FIX_TYPE_2D_FIX</td><td>2D position</td></tr>
              <tr><td>3</td><td>GPS_FIX_TYPE_3D_FIX</td><td>3D position</td></tr>
              <tr><td>4</td><td>GPS_FIX_TYPE_DGPS</td><td>DGPS/SBAS aided 3D position</td></tr>
              <tr><td>5</td><td>GPS_FIX_TYPE_RTK_FLOAT</td><td>RTK float, 3D position</td></tr>
              <tr><td>6</td><td>GPS_FIX_TYPE_RTK_FIXED</td><td>RTK fixed, 3D position</td></tr>
              <tr><td>7</td><td>GPS_FIX_TYPE_STATIC</td><td>Static fixed (base stations)</td></tr>
              <tr><td>8</td><td>GPS_FIX_TYPE_PPP</td><td>PPP, 3D position</td></tr>
            </tbody>
          </table>
        </div>
      </div>
    </div>
  );
}

export default CsvViewer;
