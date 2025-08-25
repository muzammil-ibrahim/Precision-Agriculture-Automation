// hooks/useTractorMovement.js
import { useEffect, useRef, useState } from "react";
import { lerp } from "../utils/geometry";

// unchanged logic
function getStopPosition(start, target, slotSize) {
  const half = slotSize / 2 + 6;
  const dx = target.x - start.x;
  if (dx > 0) {
    return { x: target.x - half, y: target.y };
  } else {
    return { x: target.x + half, y: target.y };
  }
}

export default function useTractorMovement({
  points,
  speed,
  slotSize,
  view,
  setView,
}) {
  const [tractorPos, setTractorPos] = useState({ x: 0, y: 0 });
  const tractorPosRef = useRef({ x: 0, y: 0 });

  const [running, setRunning] = useState(false);
  const [currentIndex, setCurrentIndex] = useState(-1);
  const [isDrilling, setIsDrilling] = useState(false);
  const [drilledPoints, setDrilledPoints] = useState([]);
  const [shakeOffset, setShakeOffset] = useState({ x: 0, y: 0 });

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

  function drillIntoCenter(centerPoint) {
    return new Promise((resolve) => {
      const start = { x: tractorPosRef.current.x, y: tractorPosRef.current.y };
      const duration = 500;
      const startTime = performance.now();

      function step(now) {
        const t = Math.min(1, (now - startTime) / duration);
        const nx = lerp(start.x, centerPoint.x, t);
        const ny = lerp(start.y, centerPoint.y, t);

        setTractorPos({ x: nx, y: ny });
        tractorPosRef.current = { x: nx, y: ny };

        setView((v) => ({ ...v, tx: nx, ty: ny }));

        const decay = 1 - t;
        const timeFactor = (now - startTime) / 40;
        const shakeAmp = 4 * decay;
        const shakeX = Math.sin(timeFactor) * shakeAmp;
        const shakeY = Math.cos(timeFactor * 1.3) * (shakeAmp * 0.6);
        setShakeOffset({ x: shakeX, y: shakeY });

        if (t < 1) {
          requestAnimationFrame(step);
        } else {
          setView((v) => ({ ...v, tx: centerPoint.x, ty: centerPoint.y }));
          setShakeOffset({ x: 0, y: 0 });
          resolve();
        }
      }
      requestAnimationFrame(step);
    });
  }

  // unchanged loop
  useEffect(() => {
    if (!running || points.length === 0) return;
    let cancelled = false;

    async function run() {
      let startIndex = currentIndex === -1 ? 0 : currentIndex;

      for (let i = startIndex; i < points.length; i++) {
        if (cancelled) return;

        setCurrentIndex(i);
        setIsDrilling(false);

        const stopPos = getStopPosition(tractorPosRef.current, points[i], slotSize);
        await moveTo(stopPos);
        if (cancelled) return;

        await new Promise((res) => setTimeout(res, 300));

        setIsDrilling(true);
        await drillIntoCenter(points[i]);
        if (cancelled) return;

        setDrilledPoints((prev) => [...prev, points[i].id]);

        await new Promise((res) => setTimeout(res, 1500));

        setIsDrilling(false);
      }

      setRunning(false);
    }

    run();
    return () => { cancelled = true; };
  }, [running, points, currentIndex, slotSize, moveTo]);

  return {
    tractorPos,
    setTractorPos,
    tractorPosRef,
    running,
    setRunning,
    currentIndex,
    setCurrentIndex,
    isDrilling,
    drilledPoints,
    shakeOffset,
  };
}
