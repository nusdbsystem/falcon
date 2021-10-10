package test

import (
	"fmt"
	"os"
	"testing"
	"time"
)

func TestDist(t *testing.T) {

	path, _ := os.Getwd()
	currentTime := time.Now().Unix()
	fmt.Println(path, fmt.Sprintf("%d", currentTime))
}
