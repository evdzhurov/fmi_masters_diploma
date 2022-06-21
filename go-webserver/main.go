package main

import (
	"github.com/gin-contrib/static"
	"github.com/gin-gonic/gin"
	swaggerFiles "github.com/swaggo/files"
	ginSwagger "github.com/swaggo/gin-swagger"

	"edj/dist-apriori-websrv/controller"
	_ "edj/dist-apriori-websrv/docs"
)

// @title Distributed System for Apriori Data Mining
// @version 1.0
// @description Master's diploma project in Distributed Systems and Mobile Technologies at FMI Sofia University, 2021
// @contact.name Evgeniy Dzhurov
// @contact.email edzhurov@uni-sofia.bg

func main() {
	r := gin.Default()

	r.Use(static.Serve("/", static.LocalFile("../vue-app/dist", false)))

	c := controller.NewController()

	jobs := r.Group("/jobs")
	{
		jobs.POST("", c.AddJob)
		jobs.GET("", c.ListJobs)
		jobs.DELETE(":id", c.DeleteJob)
	}

	workers := r.Group("/workers")
	{
		workers.POST("", c.AddWorker)
		workers.GET("", c.ListWorkers)
		workers.DELETE(":id", c.DeleteWorker)
	}

	r.GET("/swagger/*any", ginSwagger.WrapHandler(swaggerFiles.Handler))

	r.Run()
}
