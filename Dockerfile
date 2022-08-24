# syntax=docker/dockerfile:1

# Build static HTML using NodeJS.
FROM node:18 AS html-build
WORKDIR /html
COPY html/package.json html/package-lock.json ./
RUN npm install
COPY html .
RUN npm run build

# Compile lookup-min from C++ source code.
FROM alpine:latest AS bin-build
RUN apk add --no-cache \
    build-base \
    libressl-dev \
    xz-dev
WORKDIR /build
COPY Makefile* ./
COPY srcs/ ./srcs/
RUN make -j lookup-min

# Build final image with nginx running on Alpine Linux.
FROM nginx:alpine
WORKDIR /app
COPY --link minimized.bin.xz ./
COPY lookup-min-http-server.py ./
COPY dist/startup.sh ./
RUN chmod +x ./startup.sh
COPY dist/nginx-default.conf /etc/nginx/conf.d/default.conf
COPY --from=html-build /html/dist/ /app/static/
COPY --from=bin-build /build/lookup-min ./
RUN apk add --no-cache \
    libressl \
    python3 \
    xz
CMD ./startup.sh
