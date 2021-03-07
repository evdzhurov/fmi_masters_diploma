package controller

import (
	"net/http"

	"github.com/gin-gonic/gin"
)

// AddJob godoc
// @Summary Upload csv data
// @Tags jobs
// @Accept  json
// @Produce  json
// @Router /jobs [post]
func (c *Controller) AddJob(ctx *gin.Context) {
	ctx.JSON(http.StatusNotImplemented, "AddJob")
}

// ListJobs godoc
// @Summary List existing jobs
// @Tags jobs
// @Accept  json
// @Produce  json
// @Router /jobs [get]
func (c *Controller) ListJobs(ctx *gin.Context) {
	ctx.JSON(http.StatusNotImplemented, "ListJobs")
}

// ShowJob godoc
// @Summary Show specific job
// @Tags jobs
// @Accept  json
// @Produce  json
// @Router /jobs/{id} [get]
func (c *Controller) ShowJob(ctx *gin.Context) {
	ctx.JSON(http.StatusNotImplemented, "ShowJob")
}

// DeleteJob godoc
// @Summary Delete existing job
// @Tags jobs
// @Accept  json
// @Produce  json
// @Router /jobs/{id} [delete]
func (c *Controller) DeleteJob(ctx *gin.Context) {
	ctx.JSON(http.StatusNotImplemented, "DeleteJob")
}
