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

	// for logging and tracing
	http_logger := log.New(os.Stdout, "[http] ", log.LstdFlags)

	// run
	server := &http.Server{
		Addr:    common.CoordAddr,
		Handler: logger.HttpTracing(logger.NextRequestID)(logger.HttpLogging(http_logger)(mux)),
	}

	logger.Log.Println("[coordinator server]: Init DataBase...")
	controller.CreateTables()
	if common.JobDatabase == common.DBMySQL {
		// initialize the mysql db
		controller.CreateSysPorts()
	}

	logger.Log.Println("[coordinator server]: Create admin user...")
	controller.CreateUser()

	jobScheduler := controller.Init(nConsumer)

	done := make(chan os.Signal)
	signal.Notify(done, os.Interrupt, syscall.SIGINT, syscall.SIGTERM)

	go func() {
		<-done

		logger.Log.Println("[coordinator server]: Stop multi consumers")

		jobScheduler.StopMonitor()
		logger.Log.Println("[coordinator server]: Monitor Stopped")

		for i := 0; i < nConsumer; i++ {
			jobScheduler.StopConsumer()
		}

		//todo, this will shutdown the master thread at the same time
		// but the worker need to be stopped also??, add later ???

		logger.Log.Println("[coordinator server]: Consumer Stopped")
		if err := server.Shutdown(context.Background()); err != nil {
			logger.Log.Fatal("[coordinator server]: ShutDown the server", err)
		}
	}()

	logger.Log.Println("[coordinator server]: Starting multi consumers...")

	// multi-thread consumer

	for i := 0; i < nConsumer; i++ {

		go jobScheduler.Consume(i)
	}
	go jobScheduler.MonitorConsumers()

	logger.Log.Printf(
		"[coordinator server] listening on IP: %v, Port: %v\n",
		common.CoordIP,
		common.CoordPort)

	// ErrServerClosed is returned by the Server's Serve, ServeTLS, ListenAndServe, and ListenAndServeTLS methods
	// after a call to Shutdown or Close
	err := server.ListenAndServe()

	if err != nil {
		if err == http.ErrServerClosed {
			logger.Log.Print("[coordinator server]: Server closed under request\n", err)
		} else {
			logger.Log.Fatal("[coordinator server]: Server closed unexpected\n", err)
		}
	}
}
