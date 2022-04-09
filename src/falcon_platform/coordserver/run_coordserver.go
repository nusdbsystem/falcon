package coordserver

import (
	"context"
	"falcon_platform/common"
	"falcon_platform/coordserver/controller"
	"falcon_platform/coordserver/models"
	"falcon_platform/coordserver/router"
	"falcon_platform/coordserver/view"
	"falcon_platform/logger"
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"

	"github.com/gorilla/handlers"
)

func RunCoordServer(nConsumer int) {

	// set up views
	view.LoadTemplates("./coordserver/view/templates/*.html")

	// init database. and create connection pool
	JobDB := models.InitJobDB()
	JobDB.Connect()

	// set up HTTP server routes
	r := router.NewRouter(JobDB)

	// Set up Database
	logger.Log.Println("[coordinator server]: Init DataBase...")
	controller.CreateTables(JobDB)
	if common.Deployment == common.K8S {
		controller.CreateSysPorts(JobDB)
	}

	logger.Log.Println("[coordinator server]: Create admin user...")
	controller.CreateUser(JobDB)

	// Set up Job Scheduler
	logger.Log.Println("[coordinator server]: Starting multi consumers...")
	Driver := controller.Init(nConsumer)
	// multi-thread consumer
	for i := 0; i < nConsumer; i++ {

		go func(i int) {
			defer logger.HandleErrors()
			Driver.Consume(i, JobDB)
		}(i)

	}
	go func() {
		defer logger.HandleErrors()
		Driver.MonitorConsumers(JobDB)
	}()

	// for logging and tracing
	http_logger := log.New(os.Stdout, "[http] ", log.LstdFlags)
	http_logger.Println("HTTP Server is starting...")

	// reference: https://stackoverflow.com/questions/40985920/making-golang-gorilla-cors-handler-work
	// Where ORIGIN_ALLOWED is like `scheme://dns[:port]`, or `*` (insecure)
	headersOk := handlers.AllowedHeaders([]string{"X-Requested-With", "Content-Type"})
	originsOk := handlers.AllowedOrigins([]string{"*"})
	methodsOk := handlers.AllowedMethods([]string{"GET", "HEAD", "POST", "PUT", "OPTIONS"})

	// set up the HTTP server
	// modified from https://github.com/enricofoltran/simple-go-server/blob/master/main.go
	server := &http.Server{
		Addr: "0.0.0.0:" + common.CoordPort,
		// Pass instance of gorilla/mux in
		Handler: handlers.CombinedLoggingHandler(os.Stdout, handlers.CORS(originsOk, headersOk, methodsOk)(r)),
		// Handler:  logger.HttpTracing(logger.NextRequestID)(logger.HttpLogging(http_logger)(r)),
		ErrorLog: http_logger,
		// Good practice: enforce timeouts for servers to avoid Slowloris attacks
		ReadTimeout:  5 * time.Second,
		WriteTimeout: 10 * time.Second,
		IdleTimeout:  15 * time.Second,
	}

	// use a buffered channel for signal
	done := make(chan bool)
	// Set up channel on which to send signal notifications.
	// to accept graceful shutdowns
	quit := make(chan os.Signal, 1)
	signal.Notify(quit, os.Interrupt, syscall.SIGINT, syscall.SIGTERM, syscall.SIGKILL)

	// wait for quit message
	go func() {
		defer logger.HandleErrors()
		<-quit

		logger.Log.Println("[coordinator server]: Stop multi consumers")

		Driver.StopMonitor()
		logger.Log.Println("[coordinator server]: Monitor Stopped")

		for i := 0; i < nConsumer; i++ {
			Driver.StopConsumer()
		}

		logger.Log.Println("[coordinator server]: Consumer Stopped")

		http_logger.Println("HTTP Server is shutting down...")

		ctx, cancel := context.WithTimeout(context.Background(), 30*time.Second)
		defer cancel()

		server.SetKeepAlivesEnabled(false)
		if err := server.Shutdown(ctx); err != nil {
			logger.Log.Println("[coordinator server]: Could not gracefully shutDown the server: ", err)
			http_logger.Fatalf("Could not gracefully shutdown the server: %v\n", err)
		}
		close(done)
	}()

	http_logger.Println("HTTP Server is ready to handle requests at", common.CoordAddr)
	logger.Log.Printf(
		"[coordinator server] listening on IP: %v, Port: %v\n",
		common.CoordIP,
		common.CoordPort)

	// ErrServerClosed is returned by the Server's Serve, ServeTLS, ListenAndServe, and ListenAndServeTLS methods
	// after a call to Shutdown or Close
	if err := server.ListenAndServe(); err != nil && err != http.ErrServerClosed {
		logger.Log.Printf("[coordinator server]: Could not listen on ", common.CoordAddr, " err: ", err, "\n")
		http_logger.Fatalf("Could not listen on %s: %v\n", common.CoordAddr, err)
	}

	<-done
	http_logger.Println("HTTP Server stopped")
	logger.Log.Println("[coordinator server]: Server Stopped")
	os.Exit(0)
}
