BINARY="./bin/falcon_platform"

build_linux:
	CGO_ENABLED=1 GOOS=linux GOARCH=amd64 go build -o ${BINARY} falcon_platform.go

build_mac:
	CGO_ENABLED=1 GOOS=darwin GOARCH=amd64 go build -o ${BINARY} falcon_platform.go

build_windows:
	CGO_ENABLED=1 GOOS=windows GOARCH=amd64 go build -o ${BINARY} falcon_platform.go

run:
	@go run ./

clean:
	@if [ -f ${BINARY} ] ; then rm ${BINARY} ; fi

