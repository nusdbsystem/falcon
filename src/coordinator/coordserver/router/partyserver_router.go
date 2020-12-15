package router

import (
	"coordinator/coordserver/controller"
	"coordinator/coordserver/entity"
	"coordinator/client"
	"coordinator/common"
	"net/http"
)

func ListenerAdd(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

	client.ReceiveForm(r)

	listenerAddr := r.FormValue(common.ListenerAddr)
	ListenerPort := r.FormValue(common.ListenerPortKey)

	controller.ListenerAdd(ctx, listenerAddr, ListenerPort)

}

func ListenerDelete(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

	client.ReceiveForm(r)

	listenerAddr := r.FormValue(common.ListenerAddr)

	controller.ListenerDelete(ctx, listenerAddr)

}


func GetListenerPort(w http.ResponseWriter, r *http.Request, ctx *entity.Context)  {

	defer func(){
		_=r.Body.Close()
	}()

	params := r.URL.Query()

	port := controller.GetListenerPort(params.Get(common.ListenerAddr), ctx)
	_, _ = w.Write([]byte(port))
}