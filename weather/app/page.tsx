"use client"
// import Image from "next/image";
// import styles from "./page.module.css";
// import { BarChart } from '@mui/x-charts/BarChart';
import { useEffect,useState } from 'react';
import MyComp from '@/components/testCopmp';
import axios from 'axios';

interface DataResponse {
  WindSpeed: number[];
  Temperature: number[];
  Photo: number[];
  Rain: number[];
  Humidity: number[];
}

export default function Home() {
  // const [data, setData] = useState(null);
  const [Temperature, setTemperature] = useState<number[]>([]);
  const [photo, setPhoto] = useState<number[]>([]);
  const [rain, setRain] = useState<number[]>([]);
  const [wind, setWind] = useState<number[]>([]);
  const [Humidity, setHumidity] = useState<number[]>([]);
  useEffect(() => {
    const fetchData = async () => {
    
      axios.get("https://192.168.1.16:8080/getdata").then(function(response){
        
         setWind(response.data.WindSpeed)
         setTemperature(response.data.Temperature)
         setPhoto(response.data.Photo)
         setRain(response.data.Rain)
         setHumidity(response.data.Humidity)
      
      })

    };

    fetchData(); // Initial call to fetch data immediately on mount
    const interval = setInterval(fetchData, 1000); // Set interval to fetch data every second

    return () => clearInterval(interval); // Cleanup interval on component unmount
  }, []);

  return (
    <>
  <MyComp testData={Temperature} whatIs={"Temperature"}/>
  <MyComp testData={photo} whatIs={"Light"}/>
  <MyComp testData={rain} whatIs={"Rain"}/>
  <MyComp testData={wind} whatIs={"Wind Speed"}/>
  <MyComp testData={Humidity} whatIs={"Humidity"}/>
    </>
  );
}
