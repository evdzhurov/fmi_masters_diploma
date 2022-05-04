.DEFAULT_TARGET=help

.PHONY: help
help: ## Help for building and running the Distributed Apriori Mining project.
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

.PHONY: build-docker-apriori-miner
build-docker-apriori-miner: ## Builds the apriori miner node image.
	docker build -f apriori-miner/Dockerfile ./apriori-miner -t dist-apriori-miner

.PHONY: build-docker-webserver
build-docker-webserver: ## Builds the webserver node image.
	docker build -f go-webserver/Dockerfile ./go-webserver -t dist-apriori-websrv

.PHONY: build-local-apriori-miner
build-test-apriori-miner: apriori_mpi ## Builds the apriori miner binary locally for testing.

apriori_mpi: apriori-miner/apriori_mpi.cpp
	mpicxx apriori-miner/apriori_mpi.cpp -o apriori-miner/test/apriori_mpi