// hooks/useMQTT.js
import { useEffect, useState } from "react";
import mqtt from "mqtt";

export default function useMQTT(brokerUrl, topic) {
  const [yaw, setYaw] = useState(0);

  useEffect(() => {
    const client = mqtt.connect(brokerUrl);
    client.on("connect", () => {
      client.subscribe(topic);
    });
    client.on("message", (t, message) => {
      if (t === topic) {
        setYaw(parseFloat(message.toString()));
      }
    });
    return () => client.end();
  }, [brokerUrl, topic]);

  return yaw;
}
