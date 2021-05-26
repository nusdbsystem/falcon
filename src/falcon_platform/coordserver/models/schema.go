package models

import (
	"time"
)

// NOTE: GORM pluralize struct name to snake_cases as table name

type TrainJobRecord struct {
	JobId     uint `gorm:"primaryKey;autoIncrement"`
	UserID    uint
	JobInfoID uint // id of the dsl table(jobInfotable)

	// 0: init, 1: running, 2:successful, 3: failed, 4: killed
	// TODO: change to text
	Status uint

	ErrorMsg   string `gorm:"type:varchar(256)"`
	JobResult  string `gorm:"type:varchar(4096)"`
	ExtInfo    string `gorm:"type:varchar(1024)"`
	MasterAddr string `gorm:"type:varchar(256)"`

	CreateTime time.Time `gorm:"type:datetime"`
	UpdateTime time.Time `gorm:"type:datetime"`
	DeleteTime time.Time `gorm:"type:datetime"`
}

type JobInfoRecord struct {
	Id     uint `gorm:"primaryKey;autoIncrement"`
	UserID uint

	JobName     string `gorm:"type:varchar(256)"`
	JobDecs     string `gorm:"type:varchar(4096)"`
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
	ID    uint `gorm:"primaryKey;autoIncrement"`
	JobId uint // 外键job id

	ModelName string `gorm:"type:varchar(256)"` // model名称，LR etc.
	ModelDecs string `gorm:"type:varchar(256)"` //model描述，

	IsTrained uint

	CreateTime time.Time
	UpdateTime time.Time
	DeleteTime time.Time
}

type InferenceJobRecord struct {
	ID uint `gorm:"primaryKey;autoIncrement"`

	ModelId uint //外键模型id，标明哪个模型执⾏
	JobId   uint //外键模型id，标明哪个训练job

	// 0: init, 1: running, 2:successful, 3: failed, 4: killed
	Status     uint
	MasterAddr string `gorm:"type:varchar(256)"`

	CreateTime time.Time
	UpdateTime time.Time
	DeleteTime time.Time
}
