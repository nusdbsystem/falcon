package partyserver

import (
	"context"
	"coordinator/client"
	"coordinator/common"
	"coordinator/logger"
	rt "coordinator/partyserver/router"
	"log"
	"net/http"
	"os"
)

func SetupPartyServer() {
	// host: partyserverAddr
	// ServerAddr: the addr for main http server
	// host port:  for partyserver,
	defer logger.HandleErrors()
	mux := http.NewServeMux()

	// sanity check
	mux.HandleFunc("/", common.HelloPartyServer)

	mux.HandleFunc("/"+common.SetupWorker, rt.SetupWorker())
	logger.Log.Println("SetupPartyServer: registering partyserverPort to coord", common.PartyServerPort)

	// for logging and tracing
	http_logger := log.New(os.Stdout, "http: ", log.LstdFlags)

	server := &http.Server{
		Addr:    common.PartyServerIP + ":" + common.PartyServerPort,
		Handler: common.Tracing(common.NextRequestID)(common.Logging(http_logger)(mux)),
	}

	// report addr to flow htp server
	done := make(chan os.Signal)

	go func() {
		<-done

		if err := server.Shutdown(context.Background()); err != nil {
			logger.Log.Fatal("ShutDown the server", err)
		}

		client.PartyServerDelete(common.CoordAddr, common.PartyServerIP)
	}()

	logger.Log.Printf("SetupPartyServer: connecting to coord  %s to AddPort\n", common.CoordAddr)

	err := client.AddPort(common.CoordAddr, common.PartyServerPort)

	if err != nil {
		panic("SetupPartyServer: Server closed under request, " + err.Error())
	}

	logger.Log.Printf("SetupPartyServer: PartyServerAdd %s ...retry \n", common.PartyServerIP)

	err = client.PartyServerAdd(common.CoordAddr, common.PartyServerIP, common.PartyServerPort)

	if err != nil {
		panic("SetupPartyServer: PartyServerAdd error, " + err.Error())
	}

	logger.Log.Printf(
		"[party server %v] listening on IP: %v, Port: %v\n",
		common.PartyID,
		common.PartyServerIP,
		common.PartyServerPort)
	err = server.ListenAndServe()

	if err != nil {
		if err == http.ErrServerClosed {
			logger.Log.Print("Server closed under request ", err)
		} else {
			logger.Log.Fatal("Server closed unexpected ", err)
		}
	}
}
