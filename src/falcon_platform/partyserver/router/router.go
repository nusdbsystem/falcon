package router

import (
	"falcon_platform/client"
	"falcon_platform/common"
	"falcon_platform/logger"
	"falcon_platform/partyserver/controller"
	"fmt"
	"net/http"
)

func SetupWorker() func(w http.ResponseWriter, r *http.Request) {
	return func(w http.ResponseWriter, r *http.Request) {

		// TODO: why is this via Form, and not via JSON?
		client.ReceiveForm(r)

		// this is sent from main http server
		masterAddr := r.FormValue(common.MasterAddrKey)
		workerTypeKey := r.FormValue(common.TaskTypeKey)
		jobId := r.FormValue(common.JobId)
		dataPath := r.FormValue(common.TrainDataPath)
		modelPath := r.FormValue(common.ModelPath)
		dataOutput := r.FormValue(common.TrainDataOutput)

		go func() {
			defer logger.HandleErrors()
			controller.SetupWorker(masterAddr, workerTypeKey, jobId, dataPath, modelPath, dataOutput)
		}()

	}
}

// sanity check
func HelloPartyServer(w http.ResponseWriter, req *http.Request) {
	w.WriteHeader(http.StatusOK)
	fmt.Fprintf(w, "hello from falcon party server~\n")
}
