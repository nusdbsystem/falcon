package router

import (
	"falcon_platform/client"
	"falcon_platform/common"
	"falcon_platform/coordserver/controller"
	"falcon_platform/coordserver/entity"
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

func GetPartyServerPort(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {

	defer func() {
		_ = r.Body.Close()
	}()

	params := r.URL.Query()

	port := controller.GetPartyServerPort(params.Get(common.PartyServerAddrKey), ctx)
	_, _ = w.Write([]byte(port))
}
