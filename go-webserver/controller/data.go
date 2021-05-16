package controller

import (
	"net/http"

	"github.com/gin-gonic/gin"
)

// AddData godoc
// @Summary Upload csv data
// @Tags data
// @Accept  json
// @Produce  json
// @Router /data [post]
func (c *Controller) AddData(ctx *gin.Context) {
	ctx.JSON(http.StatusNotImplemented, "AddData")
}

// ListData godoc
// @Summary List csv data
// @Tags data
// @Accept  json
// @Produce  json
// @Router /data [get]
func (c *Controller) ListData(ctx *gin.Context) {
	ctx.HTML(http.StatusOK, "data", gin.H{})
}

// ShowData godoc
// @Summary Show specific csv data
// @Tags data
// @Accept  json
// @Produce  json
// @Router /data/{id} [get]
func (c *Controller) ShowData(ctx *gin.Context) {
	ctx.HTML(http.StatusOK, "data_details", gin.H{})
}

// DeleteData godoc
// @Summary Delete existing csv data
// @Tags data
// @Accept  json
// @Produce  json
// @Router /data/{id} [delete]
func (c *Controller) DeleteData(ctx *gin.Context) {
	ctx.JSON(http.StatusNotImplemented, "DeleteData")
}
