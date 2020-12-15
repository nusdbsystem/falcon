package router

import (
	"coordinator/api/controller"
	"coordinator/api/entity"
	"coordinator/client"
	"coordinator/common"
	"fmt"
	"net/http"
	"strconv"
)

func AssignPort(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

	port := controller.AssignPort(ctx)

	portStr := fmt.Sprintf("%d", port)

	_, _ = w.Write([]byte(portStr))

}

func AddPort(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {
	client.ReceiveForm(r)

	port := r.FormValue(common.AddPort)

	Port, _:= strconv.Atoi(port)

	controller.AddPort(uint(Port),ctx)

}