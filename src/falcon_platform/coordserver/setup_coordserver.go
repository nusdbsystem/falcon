package coordserver

import (
	"context"
	"falcon_platform/common"
	"falcon_platform/coordserver/controller"
	"falcon_platform/coordserver/middleware"
	"falcon_platform/coordserver/router"
	"falcon_platform/coordserver/view"
	"falcon_platform/logger"
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"
)

func SetupCoordServer(nConsumer int) {
	defer logger.HandleErrors()

	// set up views
	view.LoadTemplates("./coordserver/view/templates/*.html")

	// set up HTTP server routes
	mux := http.NewServeMux()

	middleware.SysLvPath = []string{common.Register, common.PartyServerAdd}

	// match html views to routes
	mux.HandleFunc("/submit-train-job", middleware.AddRouter(router.HtmlSubmitTrainJob, http.MethodGet))

	// REST APIs
	//job
	mux.HandleFunc("/"+common.SubmitJob, middleware.AddRouter(router.JobSubmit, http.MethodPost))
	mux.HandleFunc("/"+common.StopJob, middleware.AddRouter(router.JobKill, http.MethodPost))
	mux.HandleFunc("/"+common.UpdateTrainJobMaster, middleware.AddRouter(router.JobUpdateMaster, http.MethodPost))
	mux.HandleFunc("/"+common.UpdateJobStatus, middleware.AddRouter(router.JobUpdateStatus, http.MethodPost))
	mux.HandleFunc("/"+common.UpdateJobResInfo, middleware.AddRouter(router.JobUpdateResInfo, http.MethodPost))

	mux.HandleFunc("/"+common.QueryJobStatus, middleware.AddRouter(router.JobStatusQuery, http.MethodGet))

	//party server
	mux.HandleFunc("/"+common.Register, middleware.AddRouter(router.UserRegister, http.MethodPost))
	mux.HandleFunc("/"+common.PartyServerAdd, middleware.AddRouter(router.PartyServerAdd, http.MethodPost))
	mux.HandleFunc("/"+common.PartyServerDelete, middleware.AddRouter(router.PartyServerDelete, http.MethodPost))
	mux.HandleFunc("/"+common.GetPartyServerPort, middleware.AddRouter(router.GetPartyServerPort, http.MethodGet))

	// model serving
	mux.HandleFunc("/"+common.ModelUpdate, middleware.AddRouter(router.ModelUpdate, http.MethodPost))

	// prediction service
	mux.HandleFunc("/"+common.UpdateInferenceJobMaster, middleware.AddRouter(router.InferenceUpdateMaster, http.MethodPost))
	mux.HandleFunc("/"+common.InferenceUpdate, middleware.AddRouter(router.UpdateInference, http.MethodPost))
	mux.HandleFunc("/"+common.InferenceCreate, middleware.AddRouter(router.CreateInference, http.MethodPost))
	mux.HandleFunc("/"+common.InferenceStatusUpdate, middleware.AddRouter(router.UpdateInferenceStatus, http.MethodPost))

	// resource
	mux.HandleFunc("/"+common.AssignPort, middleware.AddRouter(router.AssignPort, http.MethodGet))
	mux.HandleFunc("/"+common.AddPort, middleware.AddRouter(router.AddPort, http.MethodPost))

	// Set up Database
	logger.Log.Println("[coordinator server]: Init DataBase...")
	controller.CreateTables()
	if common.JobDatabase == common.DBMySQL {
		// initialize the mysql db
		controller.CreateSysPorts()
	}

	logger.Log.Println("[coordinator server]: Create admin user...")
	controller.CreateUser()

	// Set up Job Scheduler
	logger.Log.Println("[coordinator server]: Starting multi consumers...")
	jobScheduler := controller.Init(nConsumer)
	// multi-thread consumer
	for i := 0; i < nConsumer; i++ {

		go jobScheduler.Consume(i)
	}
	go jobScheduler.MonitorConsumers()

	// for logging and tracing
	http_logger := log.New(os.Stdout, "[http] ", log.LstdFlags)
	http_logger.Println("HTTP Server is starting...")

	// set up the HTTP server
	// modified from https://github.com/enricofoltran/simple-go-server/blob/master/main.go
	server := &http.Server{
		Addr:     common.CoordAddr,
		Handler:  logger.HttpTracing(logger.NextRequestID)(logger.HttpLogging(http_logger)(mux)),
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

		logger.Log.Println("[coordinator server]: Stop multi consumers")

		jobScheduler.StopMonitor()
		logger.Log.Println("[coordinator server]: Monitor Stopped")

		for i := 0; i < nConsumer; i++ {
			jobScheduler.StopConsumer()
		}

		logger.Log.Println("[coordinator server]: Consumer Stopped")

		http_logger.Println("HTTP Server is shutting down...")

		ctx, cancel := context.WithTimeout(context.Background(), 30*time.Second)
		defer cancel()

		server.SetKeepAlivesEnabled(false)
		if err := server.Shutdown(ctx); err != nil {
			http_logger.Fatalf("Could not gracefully shutdown the server: %v\n", err)
			logger.Log.Fatal("[coordinator server]: Could not gracefully shutDown the server: ", err)
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
		http_logger.Fatalf("Could not listen on %s: %v\n", common.CoordAddr, err)
		logger.Log.Fatal("[coordinator server]: Could not listen on ", common.CoordAddr, " err: ", err, "\n")
	}

	<-done
	http_logger.Println("HTTP Server stopped")
	logger.Log.Println("[coordinator server]: Server Stopped")
}
