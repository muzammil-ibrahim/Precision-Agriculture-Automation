import React, { useState, useEffect } from "react";
import Papa from "papaparse";

export default function CsvPage() {
  const [data, setData] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);

  useEffect(() => {
    fetch("/your-fastapi-endpoint.csv")
      .then((res) => res.text())
      .then((text) =>
        Papa.parse(text, {
          header: true,
          skipEmptyLines: true,
          complete: (res) => {
            setData(res.data);
            setLoading(false);
          },
          error: (err) => {
            setError(err.message);
            setLoading(false);
          },
        })
      )
      .catch((err) => {
        setError(err.message);
        setLoading(false);
      });
  }, []);

  if (loading) return <p>Loading...</p>;
  if (error) return <p style={{ color: "red" }}>Error: {error}</p>;

  const headers = Object.keys(data[0] || {});
  return (
    <div>
      <h2>CSV Data</h2>
      <table border="1">
        <thead>
          <tr>
            {headers.map((h) => (
              <th key={h}>{h}</th>
            ))}
          </tr>
        </thead>
        <tbody>
          {data.map((row, idx) => (
            <tr key={idx}>
              {headers.map((h) => (
                <td key={h}>{row[h]}</td>
              ))}
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}
