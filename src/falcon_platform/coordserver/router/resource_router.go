package router

import (
	"encoding/json"
	"falcon_platform/client"
	"falcon_platform/common"
	"falcon_platform/coordserver/controller"
	"falcon_platform/coordserver/entity"
	"net/http"
	"strconv"
)

func AssignPort(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

	query := r.URL.Query()

	portNum := query.Get("portNum")

	portNumInt, err := strconv.Atoi(portNum)

	if err != nil || portNumInt == 0 {
		panic(err)
	}

	portArray := controller.AssignPort(ctx, portNumInt)

	data, err := json.Marshal(portArray)

	if err != nil {
		panic(err)
	}

	_, _ = w.Write(data)
}

func AddPort(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {
	client.ReceiveForm(r)

	port := r.FormValue(common.AddPort)

	Port, _ := strconv.Atoi(port)

	controller.AddPort(uint(Port), ctx)
}
