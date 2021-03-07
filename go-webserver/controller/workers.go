package controller

import (
	"net/http"

	"github.com/gin-gonic/gin"
)

// AddWorker godoc
// @Summary Create worker
// @Tags workers
// @Accept  json
// @Produce  json
// @Router /workers [post]
func (c *Controller) AddWorker(ctx *gin.Context) {
	ctx.JSON(http.StatusNotImplemented, "AddWorker")
}

// ListWorkers godoc
// @Summary List existing workers
// @Tags workers
// @Accept  json
// @Produce  json
// @Router /workers [get]
func (c *Controller) ListWorkers(ctx *gin.Context) {
	ctx.JSON(http.StatusNotImplemented, "ListWorkers")
}

// ShowWorker godoc
// @Summary Show specific worker
// @Tags workers
// @Accept  json
// @Produce  json
// @Router /workers/{id} [get]
func (c *Controller) ShowWorker(ctx *gin.Context) {
	ctx.JSON(http.StatusNotImplemented, "ShowWorker")
}

// DeleteWorker godoc
// @Summary Delete existing worker
// @Tags workers
// @Accept  json
// @Produce  json
// @Router /workers/{id} [delete]
func (c *Controller) DeleteWorker(ctx *gin.Context) {
	ctx.JSON(http.StatusNotImplemented, "DeleteWorker")
}
