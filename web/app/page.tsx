'use client'; //run in web

import { useEffect, useState } from "react";
import { io } from "socket.io-client";

type Sample = { seq: number; tsMs: number; value: number};

export default function Home(){
  const [samples, setSamples] = useState<Sample[]>([]); //the memory of react
  //starts empty[], setSamples-> func for update


  //run the code at the right time
  //run once the page is load (connect to socket)
  useEffect(() => {
    const socket = io('http://localhost:4321'); //connect to the backend

    socket.on('sample', (s: Sample) => {     
      setSamples((prev) => [s, ...prev].slice(0, 15)); //add the new sample tp start
      //new is the last 15
    });

    return () => {socket.disconnect();};   //the func ret disconnect when we leave the page

  }, []);

  return (
    <main style={{ padding: 32, fontFamily: 'monospace'}}>
      <h1>Live Telemetry</h1>
      {samples.length === 0 && <p>Waiting for data...</p>}
      <ul>
        {samples.map((s) =>(
          <li key={s.seq}>#{s.seq} - value: {s.value.toFixed(2)}</li>
        ))}
      </ul>
    </main>
  );


}

