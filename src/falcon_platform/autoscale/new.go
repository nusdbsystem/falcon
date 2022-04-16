package main

import "fmt"
import "math"

func beta(x int, t int) float64 {
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

	getY := func(xf float64) float64 {
		y := -2.94587705*math.Pow(10.0, -8.0)*math.Pow(xf, 7) +
			2.50848776*math.Pow(10.0, -6.0)*math.Pow(xf, 6) - 8.78441842*math.Pow(10.0, -5.0)*math.Pow(xf, 5) +
			0.00160672440*math.Pow(xf, 4) - 0.01605*math.Pow(xf, 3) + 0.08281*math.Pow(xf, 2) - 0.2161*xf + 1.14976780
		return y
	}

	//for k, v := range mapper {
	//	xf := float64(k)
	//	fmt.Println(k, v, getY(xf))
	//}

	result := float64(t) / (getY(float64(x)) * float64(x))
	return result
}

func BruteForceSearch(W int, T float64, target string) bool {

	isEffective := false
	// prediction time
	t2 := 60
	// instance weighting time
	t3 := 30
	// feature selection time
	t4 := 80
	// VFL model training time
	t5 := 150

	classNumber := 6

	// record result
	MinWorkerNumRecord := math.MaxInt
	MinTimeUsedRecord := math.MaxFloat64
	var matchRes [5]float64

	for x2 := 1; x2 < W; x2++ {
		for x3 := 1; x3 < W; x3++ {
			for x4 := 1; x4 < W; x4++ {
				for x5 := 1; x5 < W; x5++ {
					// class parallelism
					for xc := 1; xc < (classNumber + 1); xc++ {
						// total worker
						totalWorkerUsed := 1 + x2 + x3 + xc*(x5+x4)
						if totalWorkerUsed > W {
							continue
						}
						// time used
						rs := math.Ceil(float64(classNumber) / float64(xc))
						objFuncT := max(float64(t2)/float64(x2), float64(t3)/float64(x3)) +
							(rs * (beta(x4, t4) + beta(x5, t5)))

						// must meet user's required SLO
						if objFuncT > T {
							continue
						}
						// now, it meets both worker and timer requirements;
						isEffective = true

						if target == "minLatency" {
							// record the min time used, while worker number is not in consideration.
							if objFuncT < MinTimeUsedRecord {
								MinWorkerNumRecord = totalWorkerUsed
								MinTimeUsedRecord = objFuncT
								matchRes[0] = float64(x2)
								matchRes[1] = float64(x3)
								matchRes[2] = float64(x4)
								matchRes[3] = float64(x5)
								matchRes[4] = float64(xc)
							}
						} else if target == "minWorkerNumber" {
							// record the min worker used, while worker number is not in consideration.
							if totalWorkerUsed < MinWorkerNumRecord {
								MinWorkerNumRecord = totalWorkerUsed
								MinTimeUsedRecord = objFuncT
								matchRes[0] = float64(x2)
								matchRes[1] = float64(x3)
								matchRes[2] = float64(x4)
								matchRes[3] = float64(x5)
								matchRes[4] = float64(xc)
							}
						} else {
							panic(any("Not supported"))
						}
					}
				}
			}
		}
	}
	if isEffective {
		fmt.Printf("when \nx2=%d \nx3=%d \nx4=%d \nx5=%d \nxc =%d "+
			"\nrounds45=%f timeUsed=%f, worker_used=%d \n",
			int(matchRes[0]),
			int(matchRes[1]),
			int(matchRes[2]),
			int(matchRes[3]),
			int(matchRes[4]),
			math.Ceil(float64(classNumber)/matchRes[4]), MinTimeUsedRecord, MinWorkerNumRecord)
	} else {
		//fmt.Println("Not found")
	}
	return isEffective
}

func max(a, b float64) float64 {
	if a > b {
		return a
	} else {
		return b
	}
}

func main() {

	//fmt.Println("============= Now fix deadline, for different worker number, check schedule result, ============= ")
	//target := "minLatency"
	// W is worker number, 10 to 30 worker total
	//for W := 70; W < 71; W = W + 1 {
	//	if BruteForceSearch(W, float64(70), target) {
	//		fmt.Printf("-------- when W=%f search done -------- \n", W)
	//	}
	//}

	fmt.Println("============= Now fix worker number, for different time, check schedule result, ============= ")
	// T is user's defined latency, 30s to 100s
	target := "minWorkerNumber"
	for T := 500.0; T < 501.0; T = T + 10 {
		if BruteForceSearch(100, T, target) {
			fmt.Printf("-------- when T=%f search done -------- \n", T)
		}
	}
}