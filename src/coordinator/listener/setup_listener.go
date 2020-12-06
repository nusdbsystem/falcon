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

	mux := http.NewServeMux()

	mux.HandleFunc("/"+common.SetupWorker, rt.SetupWorker())

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

		c.ListenerDelete(common.CoordURLGlobal, common.ListenURLGlobal)
	}()

	c.ListenerAdd(common.CoordURLGlobal, common.ListenURLGlobal)

	logger.Do.Println("Starting HTTP server...")
	err := server.ListenAndServe()

	if err != nil {
		if err == http.ErrServerClosed {
			logger.Do.Print("Server closed under request", err)
		} else {
			logger.Do.Fatal("Server closed unexpected", err)
		}
	}
}
