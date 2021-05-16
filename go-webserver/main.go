package main

import (
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
	router := gin.Default()

	c := controller.NewController()

	router.LoadHtmlGlob("templates/*")

	router.GET("", c.ShowDashboard)

	data := router.Group("/data/csv")
	{
		data.POST("", c.AddDataCsv)
		data.GET("", c.ListDataCsv)
		data.GET(":id", c.ShowDataCsv)
		data.DELETE(":id", c.DeleteDataCsv)
	}

	jobs := router.Group("/tasks")
	{
		jobs.POST("", c.AddTask)
		jobs.GET("", c.ListTasks)
		jobs.GET(":id", c.ShowTask)
		jobs.DELETE(":id", c.DeleteTask)
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
