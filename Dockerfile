FROM node:18 AS html-build
WORKDIR /html
COPY html/package.json html/package-lock.json ./
RUN npm install
COPY html .
RUN npm run build

FROM alpine:latest AS bin-build
RUN apk add --no-cache \
    build-base \
    libressl-dev \
    xz-dev
WORKDIR /build
COPY Makefile* ./
COPY srcs/ ./srcs/
RUN make -j lookup-min

FROM nginx:alpine
WORKDIR /app
COPY minimized.bin.xz ./
COPY lookup-min-http-server.py ./
COPY dist/nginx-default.conf /etc/nginx/conf.d/default.conf
COPY --from=html-build /html/dist/ /app/static/
COPY --from=bin-build /build/lookup-min ./
RUN apk add --no-cache \
    libressl \
    python3 \
    xz
CMD nginx && ./lookup-min-http-server.py --ipv4 --host=127.0.0.1 --port=8003 --lookup=./lookup-min --minimized=minimized.bin.xz
