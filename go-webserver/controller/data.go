package controller

import (
	"net/http"

	"github.com/gin-gonic/gin"
)

// AddDataCsv godoc
// @Summary Upload csv data
// @Tags data
// @Accept  json
// @Produce  json
// @Router /data/csv [post]
func (c *Controller) AddDataCsv(ctx *gin.Context) {
	ctx.JSON(http.StatusNotImplemented, "AddDataCsv")
}

// ListDataCsv godoc
// @Summary List csv data
// @Tags data
// @Accept  json
// @Produce  json
// @Router /data/csv [get]
func (c *Controller) ListDataCsv(ctx *gin.Context) {
	ctx.JSON(http.StatusNotImplemented, "ListDataCsv")
}

// ShowDataCsv godoc
// @Summary Show specific csv data
// @Tags data
// @Accept  json
// @Produce  json
// @Router /data/csv/{id} [get]
func (c *Controller) ShowDataCsv(ctx *gin.Context) {
	ctx.JSON(http.StatusNotImplemented, "ShowDataCsv")
}

// DeleteDataCsv godoc
// @Summary Delete existing csv data
// @Tags data
// @Accept  json
// @Produce  json
// @Router /data/csv/{id} [delete]
func (c *Controller) DeleteDataCsv(ctx *gin.Context) {
	ctx.JSON(http.StatusNotImplemented, "DeleteDataCsv")
}
