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
    <div>
      <h2>CSV Data</h2>

      {/* Control buttons */}
      <div style={{ marginBottom: "10px" }}>
        <button onClick={handleStart}>Start</button>
        <button onClick={handleStop}>Stop</button>
        <button onClick={fetchAndParseCsv}>Refresh</button>
      </div>

      {csvLoading && <p>Loading...</p>}
      {csvError && <p style={{ color: "red" }}>Error: {csvError}</p>}
      {!csvLoading && !csvError && csvData.length > 0 && (
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
      )}
    </div>
  );
}

export default CsvViewer;
