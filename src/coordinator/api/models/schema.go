package models

import (
	"time"
)

type JobRecord struct {
	JobId    uint   `gorm:"primary_key;auto_increment"`
	JobName  string `gorm:"type:varchar(256)"`
	UserID   uint
	PartyIds string `gorm:"type:varchar(1024)"`

	// 0: init, 1: running, 2:successful, 3: failed, 4: killed
	Status        uint
	TaskNum       uint
	ErrorMsg      string `gorm:"type:varchar(256)"`
	JobDecs       string `gorm:"type:varchar(4096)"`
	JobResult     string `gorm:"type:varchar(4096)"`
	IsDelete      uint
	CreateTime    time.Time `gorm:"type:datetime"`
	UpdateTime    time.Time `gorm:"type:datetime"`
	DeleteTime    time.Time `gorm:"type:datetime"`
	ExtInfo       string    `gorm:"type:varchar(4096)"`
	MasterAddress string    `gorm:"type:varchar(256)"`
}

type TaskRecord struct {
	ID         uint `gorm:"primary_key;AUTO_INCREMENT"`
	JobID      uint `gorm:"unique_index"`
	TaskId     uint
	TaskName   string `gorm:"type:varchar(256)"`
	PartyIds   string `gorm:"type:varchar(256)"`
	Status     uint
	ErrorMsg   string `gorm:"type:varchar(1024)"`
	TaskDecs   string `gorm:"type:varchar(1024)"`
	TaskResult string `gorm:"type:varchar(1024)"`
	IsDelete   uint
	CreateTime time.Time
	UpdateTime time.Time
	DeleteTime time.Time
	ExtInfo    string `gorm:"type:varchar(4096)"`
}

type ExecutionRecord struct {
	ID         uint `gorm:"primary_key;AUTO_INCREMENT"`
	TapeId     uint
	TapeName   string `gorm:"type:varchar(256)"`
	Status     uint
	ErrorMsg   string `gorm:"type:varchar(4096)"`
	TapeDecs   string `gorm:"type:varchar(4096)"`
	IsDelete   uint
	CreateTime time.Time
	UpdateTime time.Time
	DeleteTime time.Time
	ExtInfo    string `gorm:"type:varchar(4096)"`
}

type ModelRecord struct {
	ID          uint `gorm:"primary_key;AUTO_INCREMENT"`
	ModelId     uint
	ModelName   string `gorm:"type:varchar(256)"`
	ModelDecs   string `gorm:"type:varchar(256)"`
	PartyNumber uint
	PartyIds    string `gorm:"type:varchar(256)"`
	ModelData   string `gorm:"type:varchar(4096)"`
	JobId       uint
	IsPublished uint
	IsDelete    uint
	CreateTime  time.Time
	UpdateTime  time.Time
	DeleteTime  time.Time
	ExtInfo     string `gorm:"type:varchar(4096)"`
}

type ServiceRecord struct {
	ID         uint   `gorm:"primary_key;AUTO_INCREMENT"`
	JobID      uint   `gorm:"unique_index"`
	MasterAddr string `gorm:"type:varchar(256)"`
	WorkerAddr string `gorm:"type:varchar(256)"`
}

type User struct {
	UserID uint   `gorm:"primary_key;AUTO_INCREMENT"`
	Name   string `gorm:"type:varchar(256)"`
}

type Listeners struct {
	ID           uint   `gorm:"primary_key;AUTO_INCREMENT"`
	ListenerAddr string `gorm:"type:varchar(256)"`
}

type TestTable struct {
	ID   uint   `gorm:"primary_key;AUTO_INCREMENT"`
	Name string `gorm:"type:varchar(256)"`
}
