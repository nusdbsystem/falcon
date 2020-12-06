package router

import (
	"coordinator/api/controller"
	"coordinator/api/entity"
	"fmt"
	"net/http"
)

func AssignPort(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

	port := controller.AssignPort(ctx)

	portStr := fmt.Sprintf("%d", port)

	_, _ = w.Write([]byte(portStr))

}

