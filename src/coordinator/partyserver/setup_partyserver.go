package partyserver

import (
	"context"
	c "coordinator/client"
	"coordinator/common"
	rt "coordinator/partyserver/router"
	"coordinator/logger"
	"net/http"
	"os"
)

func SetupPartyServer() {
	// host: partyserverAddr
	// ServerAddr: the addr for main http server
	// host port:  for partyserver,
	defer logger.HandleErrors()
	mux := http.NewServeMux()

	mux.HandleFunc("/"+common.SetupWorker, rt.SetupWorker())
	logger.Do.Println("SetupPartyServer: registering partyserverPort to coord", common.PartyServerPort)

	server := &http.Server{
		Addr:    "0.0.0.0:" + common.PartyServerPort,
		Handler: mux,
	}
	// report addr to flow htp server
	done := make(chan os.Signal)

	go func() {

		<-done
		if err := server.Shutdown(context.Background()); err != nil {
			logger.Do.Fatal("ShutDown the server", err)
		}

		c.PartyServerDelete(common.CoordAddr, common.PartyServerIP)
	}()

	logger.Do.Printf("SetupPartyServer: connecting to coord  %s to AddPort\n", common.CoordAddr)

	err := c.AddPort(common.CoordAddr, common.PartyServerPort)

	if err!=nil{
		panic("SetupPartyServer: Server closed under request, "+err.Error())
	}

	logger.Do.Printf("SetupPartyServer: PartyServerAdd %s ...retry \n", common.PartyServerIP)

	err = c.PartyServerAdd(common.CoordAddr, common.PartyServerIP, common.PartyServerPort)

	if err!=nil{
		panic("SetupPartyServer: PartyServerAdd error, "+err.Error())
	}

	logger.Do.Println("Starting HTTP server...")
	err = server.ListenAndServe()

	if err != nil {
		if err == http.ErrServerClosed {
			logger.Do.Print("Server closed under request ", err)
		} else {
			logger.Do.Fatal("Server closed unexpected ", err)
		}
	}
}
