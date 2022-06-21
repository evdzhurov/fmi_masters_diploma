package controller

import (
	"net/http"
	"strconv"

	"github.com/gin-gonic/gin"
)

// AddJob godoc
// @Summary Add new job
// @Tags jobs
// @Accept  json
// @Produce  json
// @Router /jobs [post]
func (c *Controller) AddJob(ctx *gin.Context) {
	job := Job{id: len(c.jobs), status: "pending", data_filename: "undefined", max_k: 2, min_support: 0.5, min_confidence: 0.05}
	c.jobs = append(c.jobs, job)
	ctx.String(http.StatusCreated, ctx.FullPath()+"/"+strconv.Itoa(len(c.jobs)-1))
}

// ListJobs godoc
// @Summary List existing jobs
// @Tags jobs
// @Accept  json
// @Produce  json
// @Router /jobs [get]
func (c *Controller) ListJobs(ctx *gin.Context) {
	ctx.JSON(http.StatusOK, gin.H{"jobs": c.jobs})
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
