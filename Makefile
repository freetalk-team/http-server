# Variables
DEBUG=$(abspath debug)
ALPINE=$(abspath out/alpine)
CONFIG=$(abspath examples/config/basic.js)
ROOT=$(abspath examples/www)

USER_DB=$(abspath examples/db/user.db)

THREADS=4
PORT=8084

.PHONY: all debug alpine run run-alpine clean help

# Default target
all: debug

debug: | $(DEBUG)
	cd $(DEBUG) && make -j4
	
prod: | $(ALPINE)
	docker run -it --rm \
		-v.:/source \
		-v$(ALPINE):/build \
		-w /build \
		--name alpine_builder \
		build:alpine3.21 \
		sh -c "cmake /source -DCMAKE_BUILD_TYPE=Release && make server -j4"

run: debug
	cd $(DEBUG) && ./server 0.0.0.0 8080 ./www config.js

run-alpine:
	docker run -it --rm \
		-v$(CONFIG):/etc/http-server/config.js \
		-v$(ROOT):/www \
		-v$(ALPINE)/server:/usr/local/bin/server \
		-p$(PORT):8080 \
		--name alpine_server \
		alpine:3.21 \
		sh -c "server 0.0.0.0 8080 /www /etc/http-server/config.js $(THREADS)"

run-prod:
	docker run --rm \
		-v$(USER_DB):/app/user.db \
		-p$(PORT):8080 \
		--name server_prod \
		server:alpine3.21

run-prod-test:
	docker run -it --rm \
		-v$(USER_DB):/app/user.db \
		-p$(PORT):8080 \
		--name server_prod_test \
		--entrypoint sh \
		server:alpine3.21

clean:
	cd $(DEBUG) && make clean

# Create directories if missing
$(DEBUG):
	mkdir -p $(DEBUG)

$(ALPINE):
	mkdir -p $(ALPINE)

# Show help
help:
	@echo "Makefile targets:"
	@echo "  debug      - Build code debug
	@echo "  prod       - Build code for production
	@echo "  run        - Run server
	@echo "  run-alpine - Run server in Alpine container
	@echo "  run-prod   - Run production server
	@echo "  clean      - Clean binaries
	@echo "  all        - Build 

