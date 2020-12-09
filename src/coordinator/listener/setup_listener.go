package listener

import (
	"context"
	c "coordinator/client"
	"coordinator/common"
	rt "coordinator/listener/router"
	"coordinator/logger"
	"net/http"
	"os"
)

func SetupListener() {
	// host: listenerAddr
	// ServerAddress: the address for main http server
	// host port:  for listener,
	defer logger.HandleErrors()
	mux := http.NewServeMux()

	mux.HandleFunc("/"+common.SetupWorker, rt.SetupWorker())
	logger.Do.Println("SetupListener: registering listenerPort to coord", common.ListenerPort)

	c.AddPort(common.CoordSvcURLGlobal, common.ListenerPort)

	server := &http.Server{
		Addr:    "0.0.0.0:" + common.ListenerPort,
		Handler: mux,
	}
	// report address to flow htp server
	done := make(chan os.Signal)

	go func() {

		<-done
		if err := server.Shutdown(context.Background()); err != nil {
			logger.Do.Fatal("ShutDown the server", err)
		}

		c.ListenerDelete(common.CoordSvcURLGlobal, common.ListenAddrGlobal)
	}()

	c.ListenerAdd(common.CoordSvcURLGlobal, common.ListenAddrGlobal, common.ListenerPort)

	logger.Do.Println("Starting HTTP server...")
	err := server.ListenAndServe()

	if err != nil {
		if err == http.ErrServerClosed {
			logger.Do.Print("Server closed under request ", err)
		} else {
			logger.Do.Fatal("Server closed unexpected ", err)
		}
	}
}
