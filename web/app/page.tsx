'use client'; //run in web

import { useEffect, useState } from "react";
import { io } from "socket.io-client";
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer, ReferenceLine } from "recharts"; //imports the components of recharts

type Sample = { seq: number; tsMs: number; value: number}; //same as the Sample of the backend

export default function Home(){
  const [anomalies, setAnomalies] = useState<number[]>([]);
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

    socket.on('anomaly', (s: Sample) => {   
      setAnomalies((prev) => [...prev, s.seq]); //the seq save the anomalies  
    });


    return () => {socket.disconnect();};   //the func ret disconnect when we leave the page

  }, []);

  const chartData = [...samples].reverse(); //graph need from left to right

  return (
    <main style={{ padding: 32, fontFamily: 'monospace'}}>
      <h1>Live Telemetry</h1>
      {samples.length === 0 && <p>Waiting for data...</p>} {/*placement until data come */}
      <ResponsiveContainer width="100%" height={300}>
        <LineChart data={chartData}>
          <CartesianGrid strokeDasharray="3 3"/>
          <XAxis dataKey="seq"/>
          <YAxis domain={[0.8, 1.6]}/>
          <Tooltip/>
          <ReferenceLine y={1.25} stroke="red" strokeDasharray="4 4" label= "סף אנומליה"/>
          <Line 
              type="monotone" 
              dataKey="value" 
              stroke="#4f46e5" 
              dot={(props: any) => {
                  const { cx, cy, payload } = props; //the location of the sample
                  const isAnom = anomalies.includes(payload.seq); //does the seq is exception?
                  return (
                      <circle
                          key={payload.seq}
                          cx={cx} cy={cy}
                          r={isAnom ? 5:0}
                          fill={isAnom ? 'red' : 'transparent'}/>
                  );
              }} 
              isAnimationActive={false}/>
        </LineChart>
      </ResponsiveContainer>
    </main>
  );


}

