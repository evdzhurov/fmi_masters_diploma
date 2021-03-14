FROM golang:1.15 AS websrv_node

LABEL maintainer="edzhurov@uni-sofia.bg"

WORKDIR /go/src/app

COPY . .

RUN go get -d -v ./...
RUN go install -v ./...

ENV PORT=80

EXPOSE 80

CMD ["/go/bin/dist-apriori-websrv"]