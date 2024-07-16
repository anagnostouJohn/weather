
"use client";
import { BarChart } from '@mui/x-charts/BarChart';
import { useState } from 'react';
interface MyCompProps {
    testData: string;
}


const MyComp = ({ testData }: MyCompProps) => {
    const [count, setCount] = useState(0);

    return(
        <div>
        <BarChart
        xAxis={[
          {
            id: 'barCategories',
            data: ['bar A', 'bar B', 'bar C'],
            scaleType: 'band',
          },
        ]}
        series={[
          {
            data: [2, 5, 3],
          },
        ]}
        width={500}
        height={300}
      />


     
      <p>{count}</p>
      <button onClick={() => setCount(count + 1)}>Increment</button>
    </div>
    )
}

export default MyComp