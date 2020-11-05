package router

import (
	"coordinator/api/controller"
	"coordinator/api/entity"
	"coordinator/client"
	"coordinator/config"
	"net/http"
)

func ListenerAdd(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

	client.ReceiveForm(r)

	listenerAddr := r.FormValue(config.ListenerAddr)

	controller.ListenerAdd(ctx, listenerAddr)

}

func ListenerDelete(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

	client.ReceiveForm(r)

	listenerAddr := r.FormValue(config.ListenerAddr)

	controller.ListenerDelete(ctx, listenerAddr)

}
