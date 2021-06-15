package partyserver

import (
	"context"
	"falcon_platform/client"
	"falcon_platform/common"
	"falcon_platform/logger"
	"falcon_platform/partyserver/router"
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"

	"github.com/gorilla/handlers"
)

func SetupPartyServer() {
	defer logger.HandleErrors()

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
		<-quit

		http_logger.Println("HTTP Server is shutting down...")

		logger.Log.Println("SetupPartyServer: call client.PartyServerDelete")
		if err := client.PartyServerDelete(common.CoordAddr, common.PartyServerIP); err != nil {
			http_logger.Printf("Cannot Delete PartyServer for Coordinator: %v\n", err)
		}

		ctx, cancel := context.WithTimeout(context.Background(), 30*time.Second)
		defer cancel()

		server.SetKeepAlivesEnabled(false)
		if err := server.Shutdown(ctx); err != nil {
			http_logger.Fatalf("Could not gracefully shutdown the server: %v\n", err)
			logger.Log.Fatal("SetupPartyServer: Could not gracefully shutDown the server: ", err)
		}
		close(done)
	}()

	logger.Log.Printf("SetupPartyServer: connecting to Coordinator %s to AddPort\n", common.CoordAddr)
	err := client.AddPort(common.CoordAddr, common.PartyServerPort)
	if err != nil {
		panic("SetupPartyServer: failed to AddPort on Coordinator, " + err.Error())
	}

	logger.Log.Println("SetupPartyServer: PartyServerAdd ", common.PartyServerIP)
	err = client.PartyServerAdd(common.CoordAddr, common.PartyServerIP, common.PartyServerPort)
	if err != nil {
		panic("SetupPartyServer: PartyServerAdd error, " + err.Error())
	}

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
		http_logger.Fatalf("Could not listen on %s:%s: %v\n",
			common.PartyServerIP,
			common.PartyServerPort,
			err)
		logger.Log.Fatal("[party server]: Could not listen on ", common.PartyServerIP,
			common.PartyServerPort, " err: ", err, "\n")
	}

	<-done
	http_logger.Println("HTTP Server stopped")
	logger.Log.Println("[party server]: Server Stopped")
	os.Exit(0)
}
