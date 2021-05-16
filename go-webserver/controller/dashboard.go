package controller

import (
	"net/http"

	"github.com/gin-gonic/gin"
)

// ShowDashboard godoc
// @Summary Show the dashboard
// @Tags dashboard
// @Accept  json
// @Produce  json
// @Router / [get]
func (c *Controller) ShowDashboard(ctx *gin.Context) {

	ctx.HTML(http.StatusOk, "dashboard.html", gin.H{
		"title" : "This is the dashboard"
	})
}
