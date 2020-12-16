package router

import (
	"coordinator/coordserver/controller"
	"coordinator/coordserver/entity"
	"coordinator/client"
	"coordinator/common"
	"net/http"
)

func PartyServerAdd(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

	client.ReceiveForm(r)

	partyserverAddr := r.FormValue(common.PartyServerAddrKey)
	PartyServerPort := r.FormValue(common.PartyServerPortKey)

	controller.PartyServerAdd(ctx, partyserverAddr, PartyServerPort)

}

func PartyServerDelete(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

	client.ReceiveForm(r)

	partyserverAddr := r.FormValue(common.PartyServerAddrKey)

	controller.PartyServerDelete(ctx, partyserverAddr)

}


func GetPartyServerPort(w http.ResponseWriter, r *http.Request, ctx *entity.Context)  {

	defer func(){
		_=r.Body.Close()
	}()

	params := r.URL.Query()

	port := controller.GetPartyServerPort(params.Get(common.PartyServerAddrKey), ctx)
	_, _ = w.Write([]byte(port))
}