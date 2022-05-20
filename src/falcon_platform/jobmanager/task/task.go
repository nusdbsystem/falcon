package task

import (
	"os/exec"
)

// Task interface of tasks
type Task interface {
	// GetCommand init task envs (log dir etc,) and return cmd
	GetCommand() *exec.Cmd
}
