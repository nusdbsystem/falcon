



BINARY="bin/coordinator_server"


build:
	CGO_ENABLED=1 GOOS=linux GOARCH=amd64 go build -o ${BINARY} coordinator_server.go

run:
	@go run ./


clean:
	@if [ -f ${BINARY} ] ; then rm ${BINARY} ; fi

