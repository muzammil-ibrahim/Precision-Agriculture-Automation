import React, { useState, useEffect, useRef } from "react";
import Papa from "papaparse";
import { UncontrolledReactSVGPanZoom } from 'react-svg-pan-zoom';
import tractorTop from "./TractorTop.png";
import drillIcon from "./drill.png";
import North from "./North.png";
import CsvViewer from "./CsvViewer";
import mqtt from "mqtt";


function bbox(points) {
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

function lerp(a, b, t) {
  return a + (b - a) * t;
}

export default function App() {
  const [geofence, setGeofence] = useState([]);
  const [points, setPoints] = useState([]);
  //const [running, setRunning] = useState(false);
  const [followLive, setFollowLive] = useState(false); // NEW: live-follow toggle (replaces sim "running")
  const [speed] = useState(50);
  const [currentIndex, setCurrentIndex] = useState(-1);
  const [isDrilling, setIsDrilling] = useState(false);
  const [tractorPos, setTractorPos] = useState({ x: 0, y: 0 });
  const [view, setView] = useState({ tx: 0, ty: 0, scale: 1 });
  const [drilledPoints, setDrilledPoints] = useState([]);
  // NEW: shake offset for visible shaking of the icon (in screen pixels)
  const [shakeOffset, setShakeOffset] = useState({ x: 0, y: 0 });

  const [colSpacing, setColSpacing] = useState(0);
  const [rowSpacing, setRowSpacing] = useState(0);
  const [borderMargin, setBorderMargin] = useState(0);

  const [hoveredPoint, setHoveredPoint] = useState(null);
  const [mousePos, setMousePos] = useState({x:0, y:0});

  const [yaw, setYaw] = useState(0);
  const tractorPosRef = useRef({ x: 0, y: 0 });

  const [page, setPage] = useState("field"); // “field” or “csv”

  const targetPosRef = useRef({ x: 0, y: 0 });

  const slotSize = 48;
  const FRAME_W = 900;
  const FRAME_H = 600;



  const fetchCSVData = () => {
    Promise.all([
      fetch('/get_csv').then((res) => res.text()),
      fetch('/get_csv1').then((res) => res.text()),
    ]).then(([geofenceCSV, pointsCSV]) => {
      Papa.parse(geofenceCSV, {
        header: true,
        skipEmptyLines: true,
        complete: (res) => {
          setGeofence(
            res.data.map((r, i) => ({
              id: r.id ?? String(i + 1),
              x: +r.x,
              y: +r.y,
            }))
          );
        },
      });

      Papa.parse(pointsCSV, {
        header: true,
        skipEmptyLines: true,
        complete: (res) => {
          const loaded = res.data.map((r, i) => ({
            id: r.id ?? String(i + 1),
            x: +r.x,
            y: +r.y,
          }));
          setPoints(applyZigzagOrder(loaded));
        },
      });
    });
  };

  useEffect(() => {
    fetchCSVData();
  }, []);

  useEffect(() => {
    if (geofence.length > 0) {
      const bb = bbox(geofence);
      if (tractorPos.x === 0 && tractorPos.y === 0) {
        const initPos = {
          x: bb.minX + bb.width / 2,
          y: bb.minY + bb.height / 2,
        };
        //setTractorPos(initPos);
        //tractorPosRef.current = initPos;
      }
      const scaleX = FRAME_W / Math.max(1, bb.width || 1);
      const scaleY = FRAME_H / Math.max(1, bb.height || 1);
      const scale = Math.min(scaleX, scaleY);
      setView((v) => ({ ...v, scale }));
    }
  }, [geofence]);

  useEffect(() => {
  const protocol = window.location.protocol === "https:" ? "wss" : "ws";
  const host = window.location.host; // e.g. "mydroneapi.com" or "localhost:3000"
  
  const ws = new WebSocket(`${protocol}://${host.replace(":3000", ":8000")}/ws/yaw`);
    ws.onmessage = (event) => {
      const data = JSON.parse(event.data);
      setYaw(data.yaw);  // yaw in degrees
    };
    return () => ws.close();
  }, []);

  // Open WS only while following live; push latest to targetPosRef
 useEffect(() => {
   if (!followLive) return;
   const protocol = window.location.protocol === "https:" ? "wss" : "ws";
   const host = window.location.host;
   const ws = new WebSocket(`${protocol}://${host.replace(":3000", ":8000")}/ws/location`);
   ws.onmessage = (event) => {
     const msg = JSON.parse(event.data);
     // Prefer the backend to send {x, y} already in field coords.
     // If you still receive {lat, lon}, convert here first.
     const x = msg.x;
     const y = msg.y;
     targetPosRef.current = { x, y };
   };
   ws.onclose = () => setFollowLive(false);
   ws.onerror = () => setFollowLive(false);
   return () => ws.close();
 }, [followLive]);

  // Helper: compute a stop position just outside the slot depending on approach direction
  function getStopPosition(start, target, slotSize) {
    const half = slotSize / 2 + 6; // stop a little outside the slot
    const dx = target.x - start.x;
    const dy = target.y - start.y;
    const absDx = Math.abs(dx);
    const absDy = Math.abs(dy);

      // horizontal approach
      if (dx > 0) {
        // approaching target from left -> stop to the left of the slot
        return { x: target.x - half, y: target.y };
      } else {
        // approaching from right -> stop to the right of the slot
        return { x: target.x + half, y: target.y };
      }
    }
      

  // Main movement loop (fixed for panel updates)
  // useEffect(() => {
  //   if (!running || points.length === 0) return;
  //   let cancelled = false;

  //   async function run() {
  //     let startIndex = currentIndex === -1 ? 0 : currentIndex;

  //     for (let i = startIndex; i < points.length; i++) {
  //       if (cancelled) return;

  //       setCurrentIndex(i);
  //       setIsDrilling(false);

  //       // Approach: move to stop position just outside the slot
  //       const stopPos = getStopPosition(tractorPosRef.current, points[i], slotSize);
  //       await moveTo(stopPos);
  //       if (cancelled) return;
  //       await new Promise((res) => setTimeout(res, 300));
  //       // Enter drilling mode (icon switches to drill)
  //       setIsDrilling(true);

  //       // Move into the center with shake (drill adjusting)
  //       await drillIntoCenter(points[i]);
  //       if (cancelled) return;

  //       // mark drilled and keep the drill animation for the remainder pause
  //       setDrilledPoints((prev) => [...prev, points[i].id]);
  //       // keep it drilling (small pause) — original was 1500ms
  //       await new Promise((res) => setTimeout(res, 1500));

  //       setIsDrilling(false);
  //     }

  //     setRunning(false);
  //   }

  //   run();
  //   return () => {
  //     cancelled = true;
  //   };
  //   // Intentionally only depend on running to preserve original behavior
  // }, [running]);


useEffect(() => {
   if (!followLive) return;
   let frameId;
   const loop = () => {
     const current = tractorPosRef.current;
     const target = targetPosRef.current;
     // If first fix, snap to it
     const nx = isFinite(current.x) ? lerp(current.x, target.x, 0.2) : target.x;
     const ny = isFinite(current.y) ? lerp(current.y, target.y, 0.2) : target.y;
     setTractorPos({ x: nx, y: ny });
     tractorPosRef.current = { x: nx, y: ny };
     setView((v) => ({ ...v, tx: nx, ty: ny })); // keep camera centered on tractor
     frameId = requestAnimationFrame(loop);
   };
   frameId = requestAnimationFrame(loop);
   return () => cancelAnimationFrame(frameId);
 }, [followLive]);



  function moveTo(target) {
    return new Promise((resolve) => {
      const start = { x: tractorPosRef.current.x, y: tractorPosRef.current.y };
      const dx = target.x - start.x;
      const dy = target.y - start.y;
      const dist = Math.sqrt(dx * dx + dy * dy) || 0.0001;

      const baseDuration = (dist / Math.max(1, speed)) * 1000;
      const duration = Math.max(300, baseDuration);

      const startTime = performance.now();

      function step(now) {
        const t = Math.min(1, (now - startTime) / duration);
        const nx = lerp(start.x, target.x, t);
        const ny = lerp(start.y, target.y, t);

        setTractorPos({ x: nx, y: ny });
        tractorPosRef.current = { x: nx, y: ny };

        setView((v) => ({
          ...v,
          tx: lerp(v.tx, nx, 0.15),
          ty: lerp(v.ty, ny, 0.15),
        }));

        if (t < 1) {
          requestAnimationFrame(step);
        } else {
          setView((v) => ({ ...v, tx: target.x, ty: target.y }));
          resolve();
        }
      }

      requestAnimationFrame(step);
    });
  }

  // New: drillIntoCenter — animate camera into center while applying a visible shake on the image
  function drillIntoCenter(centerPoint) {
    return new Promise((resolve) => {
      const start = { x: tractorPosRef.current.x, y: tractorPosRef.current.y };
      const duration = 500; // short entry into box
      const startTime = performance.now();

      function step(now) {
        const t = Math.min(1, (now - startTime) / duration);
        // camera/world position that will be centered (so the tractor icon appears to move)
        const nx = lerp(start.x, centerPoint.x, t);
        const ny = lerp(start.y, centerPoint.y, t);

        // update base tractor position (world coordinates)
        setTractorPos({ x: nx, y: ny });
        tractorPosRef.current = { x: nx, y: ny };

        // move camera exactly (no smoothing here so the motion is direct)
        setView((v) => ({ ...v, tx: nx, ty: ny }));

        // compute shake in screen pixels — larger at start, decaying to 0
        const decay = 1 - t; // decays to zero
        const timeFactor = (now - startTime) / 40;
        const shakeAmp = 4 * decay; // max ~4px
        const shakeX = Math.sin(timeFactor) * shakeAmp;
        const shakeY = Math.cos(timeFactor * 1.3) * (shakeAmp * 0.6);

        setShakeOffset({ x: shakeX, y: shakeY });

        if (t < 1) {
          requestAnimationFrame(step);
        } else {
          // ensure final aligned position and clear shake
          setView((v) => ({ ...v, tx: centerPoint.x, ty: centerPoint.y }));
          setShakeOffset({ x: 0, y: 0 });
          resolve();
        }
      }

      requestAnimationFrame(step);
    });
  }

  function polygonPath(pointsArr) {
    if (!pointsArr.length) return "";
    return (
      pointsArr
        .map((p, i) => `${i === 0 ? "M" : "L"}${p.x},${p.y}`)
        .join(" ") + " Z"
    );
  }

  function getColumnCount(points) {
  // Get sorted unique X coordinates
  const uniqueXs = [...new Set(points.map(p => p.x))].sort((a, b) => a - b);
  return uniqueXs.length;
}


  function applyZigzagOrder(list) {
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

const handleGenerate = async () => {
    try {
      const url = `/generate?col_spacing_ft=${colSpacing}&row_spacing_ft=${rowSpacing}&border_margin_ft=${borderMargin}`;
      const res = await fetch(url);
      const data = await res.json();
      if (data.status === "success") {
        console.log(data.points);
        fetchCSVData();
        // window.location.reload();
      } else {
        alert("Error: " + data.message);
      }
    } catch (err) {
      console.error(err);
    }
  };

  const handleClear = async () => {
  try {
    const res = await fetch("/clear-csv");
    const data = await res.json();
    console.log("Backend response:", data);

    if (data.status === "success") {
      console.log("Clear worked, reloading...");
      fetchCSVData();
      // window.location.reload();
    } else {
      alert("Error: " + data.message);
    }
  } catch (err) {
    console.error("Clear error:", err);
    alert("Failed to clear CSV files");
  }
};

const handleStartMission = async () => {
  try {
    const res = await fetch("/start_mission");
    const data = await res.json();
    console.log(data);
  } catch (err) {
    console.error("Error starting mission:", err);
  }
};



  return (
  <div>
  <nav style={{ marginBottom: "1rem" }}>
  <button onClick={() => { setPage("field"); }}>
    Agri Track
  </button>
  <button onClick={() => {
    setPage("csv");
    if (csvData.length === 0 && !csvLoading) fetchAndParseCsv();
  }}>
    Geofence Capture
  </button>
  </nav>

  {page === "field" ? (

  <div
    style={{
      display: "flex",
      background: "#fcf8f8ff",
      height: "100vh",
      width: "100vw",
      padding: "1rem",
      gap: "1rem",
      boxSizing: "border-box",
    }}
  >
    {/* FIELD */}
    <div
      className="field-container"
      style={{
        flex: 1, // fills remaining space
        display: "flex",
        alignItems: "stretch",
        position: "relative",
      }}
    >
      <UncontrolledReactSVGPanZoom
        width="100%"       // fills horizontally
        height="100%"      // fills vertically
        tool="auto"
        detectAutoPan={false}
        background="#fff"
        scaleFactor={1.2}  // smooth zoom
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
              <g key={p.id}
              onMouseEnter={(e) => {
              setHoveredPoint(p);
              setMousePos({ x: e.clientX, y: e.clientY }); // capture starting mouse pos
              }}
              onMouseLeave={() => setHoveredPoint(null)}
              onMouseMove={(e) => {
              setMousePos({ x: e.clientX, y: e.clientY }); // update as mouse moves
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
          <image
            href={isDrilling ? drillIcon : tractorTop}
            x={FRAME_W / 2 - 18 + (shakeOffset.x || 0)}
            y={FRAME_H / 2 - 18 + (shakeOffset.y || 0)}
            width="30"
            height="30"
            transform={`rotate(${yaw +90}, ${FRAME_W / 2}, ${FRAME_H / 2})`}
          />
        </svg>
        
      </UncontrolledReactSVGPanZoom>
      <img
          src= {North}  // 👈 your icon path
          alt="Direction Overlay"
          style={{
          position: "absolute",
          top: "10px",
          left: "10px",
          width: "70px",
          height: "70px",
          pointerEvents: "none",   // doesn’t block pan/zoom
        }}
      />
      {hoveredPoint && (
        <div
        style={{
      color:"black",
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
    ID: {hoveredPoint.id}<br/>
    Plant Type: Papaya
    </div>
    )}
    </div>

    {/* STATUS PANEL */}
    <div
      style={{
    minWidth: "350px",
    background: "#fff",
    padding: "1rem",
    borderRadius: "8px",
    boxShadow: "0 2px 8px rgba(0,0,0,0.1)",
    color: "#000",            // Ensure text is visible
    flexShrink: 0,            // Prevent panel from shrinking
    overflowY: "auto",        // Allow scroll if content exceeds height
  }}
  >
  <h3>Status Panel</h3>
  {points.length === 0 ? (
    <p>Loading data...</p>
  ) : (
    <>
      {/* <p>
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
      </p> */}
      <p><b>Mode:</b> {followLive ? "Live Location" : "Idle"}</p>
    </>
  )}  

    <div>
      <label>
        Column Spacing (ft):
        <input type="number" value={colSpacing} onChange={e => setColSpacing(e.target.value)} 
        style={{
    backgroundColor: "white",
    color: "black",
    border: "1px solid #ccc",
    padding: "8px",
    borderRadius: "4px"
  }} />
      </label><br/>
      <label>
        Row Spacing (ft):
        <input type="number" value={rowSpacing} onChange={e => setRowSpacing(e.target.value)}
        style={{
    backgroundColor: "white",
    color: "black",
    border: "1px solid #ccc",
    padding: "8px",
    borderRadius: "4px"
  }} />
      </label><br/>
      <label>
        Border Margin (ft):
        <input type="number" value={borderMargin} onChange={e => setBorderMargin(e.target.value)}
        style={{
    backgroundColor: "white",
    color: "black",
    border: "1px solid #ccc",
    padding: "8px",
    borderRadius: "4px"
  }} />
      </label><br/>
      <button onClick={handleGenerate}>Generate</button> <br/><br/>
      <button onClick={handleClear}>Clear Data</button> <br/><br/>
      <button onClick={handleStartMission}>Start Mission</button> <br/><br/>
    </div>
      <button
          onClick={() => {
     // toggles live follow
          setFollowLive(v => !v);
          }}
      >
      {followLive ? "Pause Live" : "Follow Live"}
      </button><br/>
    </div>
  </div>) :
  (
    <CsvViewer/>
    )}
</div>
);
}






































// import React, { useState, useEffect, useRef } from "react";
// import Papa from "papaparse";
// import { UncontrolledReactSVGPanZoom } from 'react-svg-pan-zoom';
// import { useResizeDetector } from "react-resize-detector";
// import tractorTop from "./tractor.png";
// import drillIcon from "./drill.png";
// import CsvViewer from "./CsvViewer";
// import mqtt from "mqtt";


// function bbox(points) {
//   const xs = points.map((p) => p.x);
//   const ys = points.map((p) => p.y);
//   return {
//     minX: Math.min(...xs),
//     maxX: Math.max(...xs),
//     minY: Math.min(...ys),
//     maxY: Math.max(...ys),
//     width: Math.max(...xs) - Math.min(...xs),
//     height: Math.max(...ys) - Math.min(...ys),
//   };
// }

// function lerp(a, b, t) {
//   return a + (b - a) * t;
// }

// export default function App() {
//   const [geofence, setGeofence] = useState([]);
//   const [points, setPoints] = useState([]);
//   const [running, setRunning] = useState(false);
//   const [speed] = useState(50);
//   const [currentIndex, setCurrentIndex] = useState(-1);
//   const [isDrilling, setIsDrilling] = useState(false);
//   const [tractorPos, setTractorPos] = useState({ x: 0, y: 0 });
//   const [view, setView] = useState({ tx: 0, ty: 0, scale: 1 });
//   const [drilledPoints, setDrilledPoints] = useState([]);
//   // NEW: shake offset for visible shaking of the icon (in screen pixels)
//   const [shakeOffset, setShakeOffset] = useState({ x: 0, y: 0 });

//   const [colSpacing, setColSpacing] = useState(0);
//   const [rowSpacing, setRowSpacing] = useState(0);
//   const [borderMargin, setBorderMargin] = useState(0);

//   const [hoveredPoint, setHoveredPoint] = useState(null);
//   const [mousePos, setMousePos] = useState({x:0, y:0});
//   const [yaw, setYaw] = useState(0);
//   const tractorPosRef = useRef({ x: 0, y: 0 });

//   const { width, height, ref } = useResizeDetector();

//   const [page, setPage] = useState("field"); // “field” or “csv”

//   const targetPosRef = useRef({ x: 0, y: 0 });

//   const slotSize = 48;
//   const FRAME_W = 900;
//   const FRAME_H = 600;



//   const fetchCSVData = () => {
//     Promise.all([
//       fetch('/get_csv').then((res) => res.text()),
//       fetch('/get_csv1').then((res) => res.text()),
//     ]).then(([geofenceCSV, pointsCSV]) => {
//       Papa.parse(geofenceCSV, {
//         header: true,
//         skipEmptyLines: true,
//         complete: (res) => {
//           setGeofence(
//             res.data.map((r, i) => ({
//               id: r.id ?? String(i + 1),
//               x: +r.x,
//               y: +r.y,
//             }))
//           );
//         },
//       });

//       Papa.parse(pointsCSV, {
//         header: true,
//         skipEmptyLines: true,
//         complete: (res) => {
//           const loaded = res.data.map((r, i) => ({
//             id: r.id ?? String(i + 1),
//             x: +r.x,
//             y: +r.y,
//           }));
//           setPoints(applyZigzagOrder(loaded));
//         },
//       });
//     });
//   };

//   useEffect(() => {
//     fetchCSVData();
//   }, []);

//   useEffect(() => {
//     if (geofence.length > 0) {
//       const bb = bbox(geofence);
//       if (tractorPos.x === 0 && tractorPos.y === 0) {
//         const initPos = {
//           x: bb.minX + bb.width / 2,
//           y: bb.minY + bb.height / 2,
//         };
//         setTractorPos(initPos);
//         tractorPosRef.current = initPos;
//       }
//       const scaleX = FRAME_W / Math.max(1, bb.width || 1);
//       const scaleY = FRAME_H / Math.max(1, bb.height || 1);
//       let scale = Math.min(scaleX, scaleY);
//       if (!isFinite(scale) || isNaN(scale)) scale = 1;
//       setView((v) => ({ ...v, scale }));
//     }
//   }, [geofence]);


//   useEffect(() => {
//   const protocol = window.location.protocol === "https:" ? "wss" : "ws";
//   const host = window.location.host; // e.g. "mydroneapi.com" or "localhost:3000"
  
//   const ws = new WebSocket(`${protocol}://${host.replace(":3000", ":8000")}/ws/yaw`);
//     ws.onmessage = (event) => {
//       const data = JSON.parse(event.data);
//       setYaw(data.yaw);  // yaw in degrees
//       console.log("yaw:",data.yaw);
//     };
//     return () => ws.close();
//   }, []);

//   const liveLocation = () => {
//   const protocol = window.location.protocol === "https:" ? "wss" : "ws";
//   const host = window.location.host; // e.g. "mydroneapi.com" or "localhost:3000"
  
//   const ws = new WebSocket(`${protocol}://${host.replace(":3000", ":8000")}/ws/location`);

//   ws.onmessage = (event) => {
//     const location = JSON.parse(event.data);
//     console.log("Rover Location:", location);
//    targetPosRef.current = { x: location.x, y: location.y };
//   };

//   return () => ws.close();
// }

//   // Helper: compute a stop position just outside the slot depending on approach direction
//   function getStopPosition(start, target, slotSize) {
//     const half = slotSize / 2 + 6; // stop a little outside the slot
//     const dx = target.x - start.x;
//     const dy = target.y - start.y;
//     const absDx = Math.abs(dx);
//     const absDy = Math.abs(dy);

//       // horizontal approach
//       if (dx > 0) {
//         // approaching target from left -> stop to the left of the slot
//         return { x: target.x - half, y: target.y };
//       } else {
//         // approaching from right -> stop to the right of the slot
//         return { x: target.x + half, y: target.y };
//       }
//     }
      

//   //Main movement loop (fixed for panel updates)
//   useEffect(() => {
//     if (!running || points.length === 0) return;
//     let cancelled = false;

//     async function run() {
//       let startIndex = currentIndex === -1 ? 0 : currentIndex;

//       for (let i = startIndex; i < points.length; i++) {
//         if (cancelled) return;

//         setCurrentIndex(i);
//         setIsDrilling(false);

//         // Approach: move to stop position just outside the slot
//         const stopPos = getStopPosition(tractorPosRef.current, points[i], slotSize);
//         await moveTo(stopPos);
//         if (cancelled) return;
//         await new Promise((res) => setTimeout(res, 300));
//         // Enter drilling mode (icon switches to drill)
//         setIsDrilling(true);

//         // Move into the center with shake (drill adjusting)
//         await drillIntoCenter(points[i]);
//         if (cancelled) return;

//         // mark drilled and keep the drill animation for the remainder pause
//         setDrilledPoints((prev) => [...prev, points[i].id]);
//         // keep it drilling (small pause) — original was 1500ms
//         await new Promise((res) => setTimeout(res, 1500));

//         setIsDrilling(false);
//       }

//       setRunning(false);
//     }

//     run();
//     return () => {
//       cancelled = true;
//     };
//     // Intentionally only depend on running to preserve original behavior
//   }, [running]);


// useEffect(() => {
//   let frameId;

//   function loop() {
//     const current = tractorPosRef.current;
//     const target = targetPosRef.current;

//     // Smooth follow (0.1 controls the smoothness)
//     const nx = lerp(current.x, target.x, 0.1);
//     const ny = lerp(current.y, target.y, 0.1);

//     setTractorPos({ x: nx, y: ny });
//     tractorPosRef.current = { x: nx, y: ny };

//     // 🚫 remove view.tx / ty updates (world stays fixed)

//     frameId = requestAnimationFrame(loop);
//   }

//   frameId = requestAnimationFrame(loop);

//   return () => cancelAnimationFrame(frameId);
// }, []);



//   function moveTo(target) {
//     return new Promise((resolve) => {
//       const start = { x: tractorPosRef.current.x, y: tractorPosRef.current.y };
//       const dx = target.x - start.x;
//       const dy = target.y - start.y;
//       const dist = Math.sqrt(dx * dx + dy * dy) || 0.0001;

//       const baseDuration = (dist / Math.max(1, speed)) * 1000;
//       const duration = Math.max(300, baseDuration);

//       const startTime = performance.now();

//       function step(now) {
//         const t = Math.min(1, (now - startTime) / duration);
//         const nx = lerp(start.x, target.x, t);
//         const ny = lerp(start.y, target.y, t);

//         setTractorPos({ x: nx, y: ny });
//         tractorPosRef.current = { x: nx, y: ny };

//         setView((v) => ({
//           ...v,
//           tx: lerp(v.tx, nx, 0.15),
//           ty: lerp(v.ty, ny, 0.15),
//         }));

//         if (t < 1) {
//           requestAnimationFrame(step);
//         } else {
//           setView((v) => ({ ...v, tx: target.x, ty: target.y }));
//           resolve();
//         }
//       }

//       requestAnimationFrame(step);
//     });
//   }

//   // New: drillIntoCenter — animate camera into center while applying a visible shake on the image
//   function drillIntoCenter(centerPoint) {
//     return new Promise((resolve) => {
//       const start = { x: tractorPosRef.current.x, y: tractorPosRef.current.y };
//       const duration = 500; // short entry into box
//       const startTime = performance.now();

//       function step(now) {
//         const t = Math.min(1, (now - startTime) / duration);
//         // camera/world position that will be centered (so the tractor icon appears to move)
//         const nx = lerp(start.x, centerPoint.x, t);
//         const ny = lerp(start.y, centerPoint.y, t);

//         // update base tractor position (world coordinates)
//         setTractorPos({ x: nx, y: ny });
//         tractorPosRef.current = { x: nx, y: ny };

//         // move camera exactly (no smoothing here so the motion is direct)
//         setView((v) => ({ ...v, tx: nx, ty: ny }));

//         // compute shake in screen pixels — larger at start, decaying to 0
//         const decay = 1 - t; // decays to zero
//         const timeFactor = (now - startTime) / 40;
//         const shakeAmp = 4 * decay; // max ~4px
//         const shakeX = Math.sin(timeFactor) * shakeAmp;
//         const shakeY = Math.cos(timeFactor * 1.3) * (shakeAmp * 0.6);

//         setShakeOffset({ x: shakeX, y: shakeY });

//         if (t < 1) {
//           requestAnimationFrame(step);
//         } else {
//           // ensure final aligned position and clear shake
//           setView((v) => ({ ...v, tx: centerPoint.x, ty: centerPoint.y }));
//           setShakeOffset({ x: 0, y: 0 });
//           resolve();
//         }
//       }

//       requestAnimationFrame(step);
//     });
//   }

//   function polygonPath(pointsArr) {
//     if (!pointsArr.length) return "";
//     return (
//       pointsArr
//         .map((p, i) => `${i === 0 ? "M" : "L"}${p.x},${p.y}`)
//         .join(" ") + " Z"
//     );
//   }

//   function getColumnCount(points) {
//   // Get sorted unique X coordinates
//   const uniqueXs = [...new Set(points.map(p => p.x))].sort((a, b) => a - b);
//   return uniqueXs.length;
// }


//   function applyZigzagOrder(list) {
//   if (list.length <= 3) return list;

//   const colCount = getColumnCount(list);
//   const rows = [];
//   for (let i = 0; i < list.length; i += colCount) {
//     rows.push(list.slice(i, i + colCount));
//   }
//   for (let i = 1; i < rows.length; i += 2) {
//     rows[i].reverse();
//   }
//   return rows.flat();
// }

// const handleLiveLoc = async () => {
//     try{
//       liveLocation();
//     }
//     catch(err){
//       console.error(err);
//     }
// };

// const handleGenerate = async () => {
//     try {
//       const url = `/generate?col_spacing_ft=${colSpacing}&row_spacing_ft=${rowSpacing}&border_margin_ft=${borderMargin}`;
//       const res = await fetch(url);
//       const data = await res.json();
//       if (data.status === "success") {
//         console.log(data.points);
//         fetchCSVData();
//         // window.location.reload();
//       } else {
//         alert("Error: " + data.message);
//       }
//     } catch (err) {
//       console.error(err);
//     }
//   };

//   const handleClear = async () => {
//   try {
//     const res = await fetch("/clear-csv");
//     const data = await res.json();
//     console.log("Backend response:", data);

//     if (data.status === "success") {
//       console.log("Clear worked, reloading...");
//       fetchCSVData();
//       // window.location.reload();
//     } else {
//       alert("Error: " + data.message);
//     }
//   } catch (err) {
//     console.error("Clear error:", err);
//     alert("Failed to clear CSV files");
//   }
// };

//   const svgRotation = yaw - 90;


//   return (
//   <div>
//   <nav style={{ marginBottom: "1rem" }}>
//   <button onClick={() => { setPage("field"); }}>
//     Agri Track
//   </button>
//   <button onClick={() => {
//     setPage("csv");
//     if (csvData.length === 0 && !csvLoading) fetchAndParseCsv();
//   }}>
//     Geofence Capture
//   </button>
//   </nav>

//   {page === "field" ? (

//   <div
//     style={{
//       display: "flex",
//       background: "#fcf8f8ff",
//       height: "100vh",
//       width: "100vw",
//       padding: "1rem",
//       gap: "1rem",
//       boxSizing: "border-box",
//     }}
//   >
//     {/* FIELD */}
//     <div ref={ref} style={{ flex: 1, height: "100%" }}>
//       {width && height && (
//       <UncontrolledReactSVGPanZoom
//         width={width} 
//         height={height}
//         tool="auto"
//         detectAutoPan={false}
//         background="#fff"
//         scaleFactor={1.2}  // smooth zoom
//       >
//         <svg
//   width={width} 
//   height={height}
//   style={{ background: "#e5e7eb", borderRadius: "8px" }}
//   onMouseMove={(e) => setMousePos({ x: e.clientX, y: e.clientY })}
// >
//   {/* world group — geofence + points */}
//   <g transform={`scale(${view.scale})`}>
//     <path
//       d={polygonPath(geofence)}
//       fill="#d1fae5"
//       stroke="#065f46"
//       strokeWidth={2 / view.scale}
//     />
//     {points.map((p) => (
//       <g
//         key={p.id}
//         onMouseEnter={(e) => {
//           setHoveredPoint(p);
//           setMousePos({ x: e.clientX, y: e.clientY });
//         }}
//         onMouseLeave={() => setHoveredPoint(null)}
//         onMouseMove={(e) => {
//           setMousePos({ x: e.clientX, y: e.clientY });
//         }}
//       >
//         <rect
//           x={p.x - slotSize / 2}
//           y={p.y - slotSize / 2}
//           width={slotSize}
//           height={slotSize}
//           fill={drilledPoints.includes(p.id) ? "#a7f3d0" : "#f20808ff"}
//           stroke="#374151"
//         />
//         <circle cx={p.x} cy={p.y} r={6} fill="#2563eb" />
//         <text
//           x={p.x}
//           y={p.y + 3}
//           fontSize={6}
//           textAnchor="middle"
//           fill="#fff"
//         >
//           {p.id}
//         </text>
//       </g>
//     ))}
//   </g>

//   {/* tractor — now moves by tractorPos */}
//   <image
//     href={isDrilling ? drillIcon : tractorTop}
//     x={tractorPos.x - 15 + (shakeOffset.x || 0)}
//     y={tractorPos.y - 15 + (shakeOffset.y || 0)}
//     width="30"
//     height="30"
//     transform={`rotate(${yaw -90}, ${FRAME_W / 2}, ${FRAME_H / 2})`}
//   />
// </svg>
//       </UncontrolledReactSVGPanZoom>
//       )}
//       {hoveredPoint && (
//         <div
//         style={{
//       color:"black",
//       position: "fixed",
//       left: mousePos.x + 10,
//       top: mousePos.y + 10,
//       background: "white",
//       border: "1px solid black",
//       padding: "4px",
//       fontSize: "20px",
//       pointerEvents: "none",
//     }}
//     >
//     ID: {hoveredPoint.id}<br/>
//     Plant Type: Papaya
//     </div>
//     )}
//     </div>

//     {/* STATUS PANEL */}
//     <div
//       style={{
//     minWidth: "350px",
//     background: "#fff",
//     padding: "1rem",
//     borderRadius: "8px",
//     boxShadow: "0 2px 8px rgba(0,0,0,0.1)",
//     color: "#000",            // Ensure text is visible
//     flexShrink: 0,            // Prevent panel from shrinking
//     overflowY: "auto",        // Allow scroll if content exceeds height
//   }}
//   >
//   <h3>Status Panel</h3>
//   {points.length === 0 ? (
//     <p>Loading data...</p>
//   ) : (
//     <>
//       <p>
//         <b>Current Point:</b>{" "}
//         {currentIndex >= 0 ? points[currentIndex]?.id : "-"}
//       </p>
//       <p>
//         <b>Status:</b>{" "}
//         {isDrilling ? "Drilling" : running ? "Moving" : "Idle"}
//       </p>
//       <p>
//         <b>Progress:</b>{" "}
//         {currentIndex >= 0 ? currentIndex + 1 : 0} / {points.length}
//       </p>
//     </>
//   )}  

//     <div>
//       <label>
//         Column Spacing (ft):
//         <input type="number" value={colSpacing} onChange={e => setColSpacing(e.target.value)} 
//         style={{
//     backgroundColor: "white",
//     color: "black",
//     border: "1px solid #ccc",
//     padding: "8px",
//     borderRadius: "4px"
//   }} />
//       </label><br/>
//       <label>
//         Row Spacing (ft):
//         <input type="number" value={rowSpacing} onChange={e => setRowSpacing(e.target.value)}
//         style={{
//     backgroundColor: "white",
//     color: "black",
//     border: "1px solid #ccc",
//     padding: "8px",
//     borderRadius: "4px"
//   }} />
//       </label><br/>
//       <label>
//         Border Margin (ft):
//         <input type="number" value={borderMargin} onChange={e => setBorderMargin(e.target.value)}
//         style={{
//     backgroundColor: "white",
//     color: "black",
//     border: "1px solid #ccc",
//     padding: "8px",
//     borderRadius: "4px"
//   }} />
//       </label><br/>
//       <button onClick={handleGenerate}>Generate</button> <br/><br/>
//       <button onClick={handleClear}>Clear Data</button> <br/><br/>
//       <button onClick={handleLiveLoc}>Fetch Live Location</button><br/><br/>
//     </div>

//       <button
//         onClick={() => {
//           if (!running && currentIndex === -1 && points.length > 0) {
//             const p0 = points[0];
//             setCurrentIndex(0);
//             setTractorPos({ x: p0.x, y: p0.y });
//             tractorPosRef.current = { x: p0.x, y: p0.y };
//             setView((v) => ({ ...v, tx: p0.x, ty: p0.y }));
//           }
//           setRunning((r) => !r);
//         }}
//       >
//         {running ? "Pause" : "Start"}
//       </button><br/>
//     </div>
//   </div>) :
//   (
//     <CsvViewer/>
//     )}
// </div>
// );
// }




