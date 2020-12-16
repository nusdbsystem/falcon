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

	partyserverUrl := r.FormValue(common.PartyServerUrlKey)
	PartyServerPort := r.FormValue(common.PartyServerPortKey)

	controller.PartyServerAdd(ctx, partyserverUrl, PartyServerPort)

}

func PartyServerDelete(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

	client.ReceiveForm(r)

	partyserverUrl := r.FormValue(common.PartyServerUrlKey)

	controller.PartyServerDelete(ctx, partyserverUrl)

}


func GetPartyServerPort(w http.ResponseWriter, r *http.Request, ctx *entity.Context)  {

	defer func(){
		_=r.Body.Close()
	}()

	params := r.URL.Query()

	port := controller.GetPartyServerPort(params.Get(common.PartyServerUrlKey), ctx)
	_, _ = w.Write([]byte(port))
}