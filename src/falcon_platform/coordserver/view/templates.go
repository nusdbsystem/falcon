package view

import (
	"html/template"
	"net/http"
)

// global templates object for routers to access
var templates *template.Template

func LoadTemplates(pattern string) {
	// parse the html and instantiate the global templates object
	templates = template.Must(template.ParseGlob(pattern))
}

func ExecuteTemplate(w http.ResponseWriter, tmpl string, data interface{}) {
	templates.ExecuteTemplate(w, tmpl, data)
}
