package router

import (
	"coordinator/client"
	"coordinator/common"
	"coordinator/listener/controller"
	"coordinator/logger"
	"net/http"
)

func SetupWorker() func(w http.ResponseWriter, r *http.Request) {
	return func(w http.ResponseWriter, r *http.Request) {

		client.ReceiveForm(r)

		// this is sent from main http server
		masterAddress := r.FormValue(common.MasterAddr)
		taskType := r.FormValue(common.TaskType)

		go func(){
			defer logger.HandleErrors()
			controller.SetupWorker(masterAddress, taskType)
		}()

	}
}
