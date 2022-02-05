package main

import (
	"net/http"

	"github.com/gin-contrib/multitemplate"
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

func createMyRenderer() multitemplate.Renderer {
	r := multitemplate.NewRenderer()
	r.AddFromFiles("dashboard", "templates/dashboard.html", "templates/base.html")
	r.AddFromFiles("data", "templates/data.html", "templates/base.html")
	r.AddFromFiles("data_details", "templates/data_details.html", "templates/base.html")
	r.AddFromFiles("tasks", "templates/tasks.html", "templates/base.html")
	r.AddFromFiles("task_details", "templates/task_details.html", "templates/base.html")
	r.AddFromFiles("workers", "templates/workers.html", "templates/base.html")
	r.AddFromFiles("worker_details", "templates/worker_details.html", "templates/base.html")
	return r
}

func main() {
	router := gin.Default()

	router.StaticFS("/volume", http.Dir("../volume"))

	c := controller.NewController()

	router.HTMLRender = createMyRenderer()

	router.GET("", c.ShowDashboard)

	data := router.Group("/data")
	{
		data.POST("", c.AddData)
		data.GET("", c.ListData)
		data.GET(":id", c.ShowData)
		data.DELETE(":id", c.DeleteData)
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
