package controller

import (
	"net/http"
	"strconv"

	"github.com/gin-gonic/gin"
)

// AddWorker godoc
// @Summary Create worker
// @Tags workers
// @Accept  json
// @Produce  json
// @Router /workers [post]
func (c *Controller) AddWorker(ctx *gin.Context) {
	worker := Worker{id: len(c.workers), status: "idle"}
	c.workers = append(c.workers, worker)
	ctx.String(http.StatusCreated, ctx.FullPath()+"/"+strconv.Itoa(len(c.workers)-1))
}

// ListWorkers godoc
// @Summary List existing workers
// @Tags workers
// @Accept  json
// @Produce  json
// @Router /workers [get]
func (c *Controller) ListWorkers(ctx *gin.Context) {
	ctx.JSON(http.StatusOK, gin.H{"workers": c.workers})
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
