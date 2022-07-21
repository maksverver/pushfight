Implementation of a web app that can be used to evaluate Push Fight positions.

It's built with NPM and React.

# To install dependencies:
npm install

# To build (output is in the dist/ directory):
npm run build

# To develop locally:
npm run serve

Note that the web app requires the position lookup RPC server
(lookup-min-http-server.py) to be running.

lookup-min-http.server.py serves at http://localhost:8003/ by default,
and the webpack dev server started with `npm run serve` will proxy requests
from http://localhost:8080/lookup/ to http://localhost:8003/.

For production setup, the proxy configuration must be configured separately.
