package utils

import (
	"time"
)

func GenerateJobId() int64 {
	return time.Now().UnixNano() / int64(time.Millisecond)
}
