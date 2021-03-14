package controller

import (
	"net/http"

	"github.com/gin-gonic/gin"
)

// AddTask godoc
// @Summary Add new task
// @Tags tasks
// @Accept  json
// @Produce  json
// @Router /tasks [post]
func (c *Controller) AddTask(ctx *gin.Context) {
	ctx.JSON(http.StatusNotImplemented, "AddTask")
}

// ListTasks godoc
// @Summary List existing tasks
// @Tags tasks
// @Accept  json
// @Produce  json
// @Router /tasks [get]
func (c *Controller) ListTasks(ctx *gin.Context) {
	ctx.JSON(http.StatusNotImplemented, "ListTasks")
}

// ShowTask godoc
// @Summary Show specific task
// @Tags tasks
// @Accept  json
// @Produce  json
// @Router /tasks/{id} [get]
func (c *Controller) ShowTask(ctx *gin.Context) {
	ctx.JSON(http.StatusNotImplemented, "ShowTask")
}

// DeleteTask godoc
// @Summary Delete existing task
// @Tags tasks
// @Accept  json
// @Produce  json
// @Router /tasks/{id} [delete]
func (c *Controller) DeleteTask(ctx *gin.Context) {
	ctx.JSON(http.StatusNotImplemented, "DeleteTask")
}
