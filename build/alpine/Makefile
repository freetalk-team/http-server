# Variables
IMAGE_NAME=build:alpine3.21
PROD_IMAGE_NAME=prod:alpine3.21
RUNTIME_IMAGE_NAME=server:alpine3.21

CONTAINER_NAME=builder
RUNTIME_CONTAINER_NAME=http_server

SERVER_BINARY=out/alpine/server
WWW_ROOT=examples/www
CONFIG=examples/config/basic.js

# Default target
all: build run

# Build Docker image
build:
	docker build -t $(IMAGE_NAME) .

prod:
	docker build \
		-f Dockerfile.prod \
		-t $(PROD_IMAGE_NAME) .

heroku:
	docker build \
		--build-arg SERVER_BINARY=$(SERVER_BINARY) \
		--build-arg WWW_ROOT=$(WWW_ROOT) \
		--build-arg CONFIG=$(CONFIG) \
		-f Dockerfile.basic \
		-t $(RUNTIME_IMAGE_NAME) ../..

# Run Docker container
run:
	docker run -it --rm --name $(CONTAINER_NAME) $(IMAGE_NAME)

# Stop and remove container
stop:
	docker stop $(CONTAINER_NAME) || true

# Clean Docker image
clean:
	docker rmi $(IMAGE_NAME) || true

# Show help
help:
	@echo "Makefile targets:"
	@echo "  build   - Build Docker image"
	@echo "  run     - Run Docker container"
	@echo "  stop    - Stop running container"
	@echo "  clean   - Remove Docker image"
	@echo "  all     - Build and run"

