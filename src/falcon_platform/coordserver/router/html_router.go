package router

import (
	"falcon_platform/coordserver/entity"
	"falcon_platform/coordserver/view"
	"net/http"
)

func HtmlSubmitTrainJob(w http.ResponseWriter, r *http.Request, ctx *entity.Context) {
	// updates, err := models.GetAllUpdates()
	// if err != nil {
	// 	utils.InternalServerError(w)
	// 	return
	// }
	view.ExecuteTemplate(w, "index.html", struct {
		Title       string
		DisplayForm bool
	}{
		Title:       "Submit Train Job",
		DisplayForm: true,
	})
}
