package router

import (
	"coordinator/client"
	"coordinator/config"
	"coordinator/listener/controller"
	"net/http"
)

func SetupWorker(httpHost string) func(w http.ResponseWriter, r *http.Request) {
	return func(w http.ResponseWriter, r *http.Request) {

		client.ReceiveForm(r)

		// this is sent from main http server
		masterAddress := r.FormValue(config.MasterAddr)
		taskType := r.FormValue(config.TaskType)

		go controller.SetupWorker(httpHost, masterAddress, taskType)

	}
}
