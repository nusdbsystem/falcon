package models

import (
	"time"
)

// NOTE: GORM pluralize struct name to snake_cases as table name
// NOTE: follow the GORM conventions for automatic configuration
// https://gorm.io/docs/conventions.html
// ID as Primary Key
// GORM pluralizes struct name to snake_cases as table name
// For models having CreatedAt field, the field will be
// set to the current time when the record is first created if its value is zero

type TrainJobRecord struct {
	JobId     uint `gorm:"primaryKey;autoIncrement"`
	UserID    uint
	JobInfoID uint // id of the dsl table(jobInfotable)

	Status string `gorm:"type:varchar(50)"`

	ErrorMsg   string `gorm:"type:varchar(256)"`
	JobResult  string `gorm:"type:varchar(4096)"`
	ExtInfo    string `gorm:"type:varchar(1024)"`
	MasterAddr string `gorm:"type:varchar(256)"`

	// TODO: change to GORM convention CreatedAt and UpdatedAt
	CreateTime time.Time `gorm:"type:datetime"`
	UpdateTime time.Time `gorm:"type:datetime"`
	DeleteTime time.Time `gorm:"type:datetime"`
}

type JobInfoRecord struct {
	Id     uint `gorm:"primaryKey;autoIncrement"`
	UserID uint

	JobName     string `gorm:"type:varchar(256)"`
	JobInfo     string `gorm:"type:varchar(4096)"`
	FlSetting   string
	ExistingKey uint
	PartyNum    uint
	PartyIds    string `gorm:"type:varchar(1024)"`
	TaskNum     uint
	TaskInfo    string `gorm:"type:varchar(1024)"`
}

type TaskRecord struct {
	ID    uint `gorm:"primaryKey;autoIncrement"`
	JobID uint `gorm:"uniqueIndex"`
	//TaskId     uint
	TaskName string `gorm:"type:varchar(256)"`
	PartyIds string `gorm:"type:varchar(256)"`

	Status string

	ErrorMsg   string `gorm:"type:varchar(1024)"`
	TaskInfo   string `gorm:"type:varchar(1024)"`
	TaskResult string `gorm:"type:varchar(1024)"`
	IsDelete   uint
	CreateTime time.Time
	UpdateTime time.Time
	DeleteTime time.Time
	ExtInfo    string `gorm:"type:varchar(4096)"`
}

type ServiceRecord struct {
	ID         uint   `gorm:"primaryKey;autoIncrement"`
	JobID      uint   `gorm:"uniqueIndex"`
	MasterAddr string `gorm:"type:varchar(256)"`
	WorkerAddr string `gorm:"type:varchar(256)"`
}

type PortRecord struct {
	ID       uint `gorm:"primaryKey;autoIncrement"`
	NodeId   uint
	Port     uint `gorm:"uniqueIndex"`
	IsDelete uint
}

type User struct {
	UserID uint   `gorm:"primaryKey;autoIncrement"`
	Name   string `gorm:"type:varchar(256)"`
}

type PartyServer struct {
	ID              uint   `gorm:"primaryKey;autoIncrement"`
	PartyServerAddr string `gorm:"type:varchar(256)"`
	Port            string `gorm:"type:varchar(256)"`
}

//////////////////////////////////////////////////////////////////
///////////////////// PartyServerDatabase  //////////////////////////
//////////////////////////////////////////////////////////////////

// TODO: what is TapeId? TapeDecs?
type ExecutionRecord struct {
	ID         uint `gorm:"primaryKey;autoIncrement"`
	TapeId     uint
	TapeName   string `gorm:"type:varchar(256)"`
	Status     string
	ErrorMsg   string `gorm:"type:varchar(4096)"`
	TapeDecs   string `gorm:"type:varchar(4096)"`
	IsDelete   uint
	CreateTime time.Time
	UpdateTime time.Time
	DeleteTime time.Time
	ExtInfo    string `gorm:"type:varchar(4096)"`
}

type ModelRecord struct {
	ID    uint `gorm:"primaryKey;autoIncrement"`
	JobId uint // 外键job id

	ModelName string `gorm:"type:varchar(256)"`
	ModelInfo string `gorm:"type:varchar(256)"`

	IsTrained uint

	CreateTime time.Time
	UpdateTime time.Time
	DeleteTime time.Time
}

type InferenceJobRecord struct {
	ID uint `gorm:"primaryKey;autoIncrement"`

	ModelId uint //外键模型id，标明哪个模型执⾏
	JobId   uint //外键模型id，标明哪个训练job

	Status     string `gorm:"type:varchar(50)"`
	MasterAddr string `gorm:"type:varchar(256)"`

	CreateTime time.Time
	UpdateTime time.Time
	DeleteTime time.Time
}
