package controller

import dist "coordinator/distributed"

func SetupWorker(httpHost, masterAddress string) {

	e := dist.SetupWorker(httpHost, masterAddress)
	if e != nil {
		panic(e)
	}

}
