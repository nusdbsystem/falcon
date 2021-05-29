package router

import (
	"falcon_platform/coordserver/view"
	"net/http"
)

func HtmlIndex(w http.ResponseWriter, r *http.Request) {
	view.ExecuteTemplate(w, "index.html", nil)
}

func HtmlSubmitTrainJob(w http.ResponseWriter, r *http.Request) {
	// updates, err := models.GetAllUpdates()
	// if err != nil {
	// 	utils.InternalServerError(w)
	// 	return
	// }
	view.ExecuteTemplate(w, "submit_train_job.html", struct {
		Title       string
		DisplayForm bool
	}{
		Title:       "Submit Train Job",
		DisplayForm: true,
	})
}

func HtmlUploadTrainJobFile(w http.ResponseWriter, r *http.Request) {
	// updates, err := models.GetAllUpdates()
	// if err != nil {
	// 	utils.InternalServerError(w)
	// 	return
	// }
	view.ExecuteTemplate(w, "upload_train_job_file.html", "token")
}
