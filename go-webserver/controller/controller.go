package controller

type Controller struct {
	workers []Worker
	jobs    []Job
}

// NewController example
func NewController() *Controller {
	return &Controller{}
}

type Worker struct {
	id     int
	status string
}

type Job struct {
	id             int
	status         string
	data_filename  string
	max_k          int
	min_support    float32
	min_confidence float32
}
