package common

type JobSubmitReply struct {
	JobId   uint   `json:"job_id"`
	JobName string `json:"job_name"`
	UserId  uint   `json:"user_id"`
	TaskNum uint   `json:"task_num,uint"`
	Status  string `json:"status"`
}

type JobStatusReply struct {
	JobId  uint   `json:"job_id"`
	Status string `json:"status"`
}

type JobIdGet struct {
	JobId string `json:"job_id"`
}

type JobModelReportReply struct {
	JobId                uint   `json:"job_id"`
	EvaluationReportPath string `json:"evaluation_report_path"`
}
