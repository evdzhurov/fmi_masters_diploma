package main

import (
	"github.com/gin-gonic/gin"
	swaggerFiles "github.com/swaggo/files"
	ginSwagger "github.com/swaggo/gin-swagger"

	"edj/apriori/controller"
	_ "edj/apriori/docs"
)

// @title Distributed System for Apriori Data Mining
// @version 1.0
// @description Master's diploma project in Distributed Systems and Mobile Technologies at FMI Sofia University, 2021
// @contact.name Evgeniy Dzhurov
// @contact.email edzhurov@uni-sofia.bg

func main() {
	router := gin.Default()

	c := controller.NewController()

	router.GET("", c.ShowDashboard)

	data := router.Group("/data/csv")
	{
		data.POST("", c.AddDataCsv)
		data.GET("", c.ListDataCsv)
		data.GET(":id", c.ShowDataCsv)
		data.DELETE(":id", c.DeleteDataCsv)
	}

	jobs := router.Group("/jobs")
	{
		jobs.POST("", c.AddJob)
		jobs.GET("", c.ListJobs)
		jobs.GET(":id", c.ShowJob)
		jobs.DELETE(":id", c.DeleteJob)
	}

	workers := router.Group("/workers")
	{
		workers.POST("", c.AddWorker)
		workers.GET("", c.ListWorkers)
		workers.GET(":id", c.ShowWorker)
		workers.DELETE(":id", c.DeleteWorker)
	}

	router.GET("/swagger/*any", ginSwagger.WrapHandler(swaggerFiles.Handler))

	router.Run() // listen and serve on 0.0.0.0:8080 (for Windows - localhost:8080)
}
