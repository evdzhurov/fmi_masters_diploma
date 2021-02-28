package main

import (
	"github.com/gin-gonic/gin"
	swaggerFiles "github.com/swaggo/files"
	ginSwagger "github.com/swaggo/gin-swagger"

	_ "edj/apriori/docs"
)

// @title Distributed Apriori API
// @version 1.0
// @description This is a master's diploma project in Distributed Systems and Mobile Technologies at FMI Sofia University

// @contact.name Evgeniy Dzhurov
// @contact.email edzhurov@uni-sofia.bg

// Test godoc
// @Summary Show an account
// @Description get string by ID
// @Tags accounts
// @Accept  json
// @Produce  json
// @Param id path int true "Account ID"
// @Router /accounts/{id} [get]
func Test(ctx *gin.Context) {
}

func main() {
	r := gin.Default()

	r.GET("/ping", func(c *gin.Context) {
		c.JSON(200, gin.H{
			"message": "pong",
		})
	})

	//url := ginSwagger.URL("http://localhost:8080/swagger/doc.json")
	r.GET("/swagger/*any", ginSwagger.WrapHandler(swaggerFiles.Handler))

	r.Run() // listen and serve on 0.0.0.0:8080 (for Windows - localhost:8080)
}
