package listener

import (
	"context"
	c "coordinator/client"
	"coordinator/config"
	rt "coordinator/listener/router"
	"log"
	"net/http"
	"os"
)

func SetupListener(host, port string, ServerAddress string) {

	// ServerAddress: the address for main http server
	// host port:  for listener,

	httpAddr := host + ":" + port
	mux := http.NewServeMux()

	mux.HandleFunc("/"+config.SetupWorker, rt.SetupWorker(host))

	server := &http.Server{
		Addr:    httpAddr,
		Handler: mux,
	}
	// report address to flow htp server
	done := make(chan os.Signal)

	go func() {

		<-done
		if err := server.Shutdown(context.Background()); err != nil {
			log.Fatal("ShutDown the server", err)
		}

		c.ListenerDelete(ServerAddress, httpAddr)
	}()

	c.ListenerAdd(ServerAddress, httpAddr)

	log.Println("Starting HTTP server...")
	err := server.ListenAndServe()

	if err != nil {
		if err == http.ErrServerClosed {
			log.Print("Server closed under request", err)
		} else {
			log.Fatal("Server closed unexpected", err)
		}
	}
}
