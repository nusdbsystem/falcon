package partyserver

import (
	"context"
	"falcon_platform/client"
	"falcon_platform/common"
	"falcon_platform/logger"
	"falcon_platform/partyserver/router"
	"falcon_platform/partyserver/singleton"

	// "falcon_platform/resourcemanager"
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"

	"github.com/gorilla/handlers"
)

func RunPartyServer() {

	// init party server cluster's status
	singleton.GetServerJobInfo()

	// set up HTTP server routes
	r := router.NewRouter()

	// for logging and tracing
	http_logger := log.New(os.Stdout, "[http] ", log.LstdFlags)
	http_logger.Println("HTTP Server is starting...")

	// set up the HTTP server
	// modified from https://github.com/enricofoltran/simple-go-server/blob/master/main.go
	server := &http.Server{
		Addr:    "0.0.0.0:" + common.PartyServerPort,
		Handler: handlers.CombinedLoggingHandler(os.Stdout, r),
		// Handler:  logger.HttpTracing(logger.NextRequestID)(logger.HttpLogging(http_logger)(r)),
		ErrorLog: http_logger,
		// Good practice: enforce timeouts for servers
		//ReadTimeout covers the time from when the connection is accepted to when the request body is fully read
		//(if you do read the body, otherwise to the end of the headers). It's implemented in net/http by calling
		//  SetReadDeadline immediately after Accept.
		//WriteTimeout normally covers the time from the end of the request header read to the end of the response write
		//  (a.k.a. the lifetime of the ServeHTTP), by calling SetWriteDeadline at the end of readRequest.
		ReadTimeout:  5 * time.Second,
		WriteTimeout: 10 * time.Second,
		IdleTimeout:  15 * time.Second,
	}

	// use a buffered channel for signal
	done := make(chan bool)
	// Set up channel on which to send signal notifications.
	quit := make(chan os.Signal, 1)
	signal.Notify(quit, os.Interrupt, syscall.SIGINT, syscall.SIGTERM, syscall.SIGKILL)

	go func() {
		defer logger.HandleErrors()

		<-quit

		http_logger.Println("HTTP Server is shutting down...")
		logger.Log.Println("RunPartyServer: call client.PartyServerDelete")
		logger.Log.Println("RunPartyServer: delete all docker containers")
		// 		dockerSdk := new(resourcemanager.DockerSdkMngr)
		// 		dockerSdk.DeleteAll()

		ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
		defer cancel()

		server.SetKeepAlivesEnabled(false)
		if err := server.Shutdown(ctx); err != nil {
			logger.Log.Println("RunPartyServer: Could not gracefully shutDown the server: ", err)
			http_logger.Fatalf("Could not gracefully shutdown the server: %v\n", err)
		}
		close(done)
	}()

	logger.Log.Printf("RunPartyServer: connecting to Coordinator %s to AddPort %s\n", common.CoordAddr, common.PartyServerPort)
	client.AddPort(common.CoordAddr, common.PartyServerPort)

	logger.Log.Println("RunPartyServer: PartyServerAdd ", common.PartyServerIP)
	client.PartyServerAdd(common.CoordAddr, common.PartyServerIP, common.PartyServerPort)

	http_logger.Printf("[party server %v] is ready to handle requests at IP: %v, Port: %v\n",
		common.PartyID,
		common.PartyServerIP,
		common.PartyServerPort)
	logger.Log.Printf(
		"[party server %v] listening on IP: %v, Port: %v\n",
		common.PartyID,
		common.PartyServerIP,
		common.PartyServerPort)

	// ErrServerClosed is returned by the Server's Serve, ServeTLS, ListenAndServe, and ListenAndServeTLS methods
	// after a call to Shutdown or Close
	if err := server.ListenAndServe(); err != nil && err != http.ErrServerClosed {
		logger.Log.Printf("[party server]: Could not listen on ", common.PartyServerIP,
			common.PartyServerPort, " err: ", err, "\n")
		http_logger.Fatalf("Could not listen on %s:%s: %v\n", common.PartyServerIP, common.PartyServerPort, err)
	}

	<-done
	http_logger.Println("HTTP Server stopped")
	logger.Log.Println("[party server]: Server Stopped")
	os.Exit(0)
}
