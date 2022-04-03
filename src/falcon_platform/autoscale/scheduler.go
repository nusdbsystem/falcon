package main

import "fmt"
import "math"

func beta(x int, t int) float64 {
	// with parallelism increasing, the latency decrease in un-linear way
	// below is get from graph https://www.researchgate.net/figure/The-expected-delay-decreases-as-the-number-of-FS-nodes-increases_fig2_305218097
	mapper := make(map[int]float64)
	mapper[1] = 1.0
	mapper[2] = 0.95
	mapper[3] = 0.92
	mapper[4] = 0.91
	mapper[5] = 0.90
	mapper[6] = 0.88
	mapper[7] = 0.85
	mapper[8] = 0.80
	mapper[9] = 0.76
	mapper[10] = 0.71
	mapper[11] = 0.68
	mapper[12] = 0.65
	mapper[13] = 0.63
	mapper[14] = 0.58
	mapper[15] = 0.54
	mapper[16] = 0.53
	mapper[17] = 0.51
	mapper[18] = 0.47
	mapper[19] = 0.43
	mapper[20] = 0.39
	result := float64(t) / (mapper[x] * float64(x))
	return result
}

func max(a, b float64) float64 {
	if a > b {
		return a
	} else {
		return b
	}
}

func BruteForceSearch(T float64) {

	// 10 worker
	W := 40

	// prediction time
	t2 := 30
	// instance weighting time
	t3 := 10
	// feature selection time
	t4 := 80
	// VFL model training time
	t5 := 50

	c := 4

	// record result
	minWorkerNum := 21
	TimeUsed := 0.0

	for x2 := 1; x2 < W; x2++ {
		for x3 := 1; x3 < W; x3++ {
			for x4 := 1; x4 < W; x4++ {
				for x5 := 1; x5 < W; x5++ {
					for xc := 1; xc < (c + 1); xc++ {
						// total worker
						totalWorkerUsed := 1 + x2 + x3 + xc*(x5+x4)
						if totalWorkerUsed > W {
							continue
						}
						// time used
						s := float64(c) / float64(xc)
						rs := math.Ceil(s)
						objFunc := max(float64(t2)/float64(x2), float64(t3)/float64(x3)) +
							(rs * (beta(x4, t4) + beta(x5, t5)))

						// if meet user's SLO requirements
						if objFunc < T {
							if totalWorkerUsed < minWorkerNum {
								minWorkerNum = totalWorkerUsed
								TimeUsed = objFunc
								fmt.Printf("when x2=%d x3=%d x4=%d x5=%d xc =%d "+
									"s=%f rs=%f timeUsed=%f, worker_used=%d \n", x2, x3, x4, x5, xc, s, rs, TimeUsed, minWorkerNum)
							}

						}
					}
				}
			}
		}

	}
}

func main() {

	// T is user's defined latency, 30s to 100s
	for T := 50.0; T < 200.0; T = T + 10 {
		BruteForceSearch(T)
		fmt.Printf("-------- when T=%f search done -------- \n", T)
	}

}

/* output:
-------- when T=50.000000 search done --------
-------- when T=60.000000 search done --------
-------- when T=70.000000 search done --------
-------- when T=80.000000 search done --------
when x2=2 x3=1 x4=2 x5=2 xc =4 s=1.000000 rs=1.000000 timeUsed=83.421053, worker_used=20
-------- when T=90.000000 search done --------
when x2=1 x3=1 x4=2 x5=2 xc =4 s=1.000000 rs=1.000000 timeUsed=98.421053, worker_used=19
when x2=2 x3=1 x4=4 x5=3 xc =2 s=2.000000 rs=2.000000 timeUsed=95.187928, worker_used=18
-------- when T=100.000000 search done --------
when x2=1 x3=1 x4=2 x5=2 xc =4 s=1.000000 rs=1.000000 timeUsed=98.421053, worker_used=19
when x2=2 x3=1 x4=2 x5=1 xc =4 s=1.000000 rs=1.000000 timeUsed=107.105263, worker_used=16
-------- when T=110.000000 search done --------
when x2=1 x3=1 x4=2 x5=2 xc =4 s=1.000000 rs=1.000000 timeUsed=98.421053, worker_used=19
when x2=1 x3=1 x4=3 x5=4 xc =2 s=2.000000 rs=2.000000 timeUsed=115.443542, worker_used=17
when x2=2 x3=1 x4=2 x5=1 xc =4 s=1.000000 rs=1.000000 timeUsed=107.105263, worker_used=16
-------- when T=120.000000 search done --------
when x2=1 x3=1 x4=1 x5=3 xc =4 s=1.000000 rs=1.000000 timeUsed=128.115942, worker_used=19
when x2=1 x3=1 x4=2 x5=1 xc =4 s=1.000000 rs=1.000000 timeUsed=122.105263, worker_used=15
when x2=2 x3=1 x4=3 x5=2 xc =2 s=2.000000 rs=2.000000 timeUsed=125.602593, worker_used=14
-------- when T=130.000000 search done --------
when x2=1 x3=1 x4=1 x5=2 xc =4 s=1.000000 rs=1.000000 timeUsed=136.315789, worker_used=15
when x2=1 x3=1 x4=5 x5=6 xc =1 s=4.000000 rs=4.000000 timeUsed=138.989899, worker_used=14
-------- when T=140.000000 search done --------
when x2=1 x3=1 x4=1 x5=2 xc =4 s=1.000000 rs=1.000000 timeUsed=136.315789, worker_used=15
when x2=1 x3=1 x4=3 x5=2 xc =2 s=2.000000 rs=2.000000 timeUsed=140.602593, worker_used=13
when x2=2 x3=1 x4=1 x5=1 xc =4 s=1.000000 rs=1.000000 timeUsed=145.000000, worker_used=12
-------- when T=150.000000 search done --------
when x2=1 x3=1 x4=1 x5=2 xc =4 s=1.000000 rs=1.000000 timeUsed=136.315789, worker_used=15
when x2=1 x3=1 x4=2 x5=3 xc =2 s=2.000000 rs=2.000000 timeUsed=150.442410, worker_used=13
when x2=1 x3=1 x4=5 x5=4 xc =1 s=4.000000 rs=4.000000 timeUsed=156.056166, worker_used=12
-------- when T=160.000000 search done --------
when x2=1 x3=1 x4=1 x5=1 xc =4 s=1.000000 rs=1.000000 timeUsed=160.000000, worker_used=11
-------- when T=170.000000 search done --------
when x2=1 x3=1 x4=1 x5=1 xc =4 s=1.000000 rs=1.000000 timeUsed=160.000000, worker_used=11
-------- when T=180.000000 search done --------
when x2=1 x3=1 x4=1 x5=1 xc =4 s=1.000000 rs=1.000000 timeUsed=160.000000, worker_used=11
-------- when T=190.000000 search done --------
*/
