package router

import (
	"falcon_platform/client"
	"falcon_platform/common"
	"falcon_platform/coordserver/controller"
	"falcon_platform/coordserver/entity"
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

	Port, _ := strconv.Atoi(port)

	controller.AddPort(uint(Port), ctx)

}
