# WebServer in C++

Minimal C++98 web server with non-blocking I/O (poll), a simple configuration parser, static file serving, basic error responses, and experimental CGI (Python) support. Developed by [@nands93](https://github.com/nands93) and [@amenesca](https://github.com/amenesca).

- Core entrypoint: [srcs/main.cpp](srcs/main.cpp)
- Server bootstrap: [`WebServer::configVServers`](srcs/WebServer/WebServer.hpp), [`WebServer::initConnection`](srcs/WebServer/WebServer.hpp)
- Socket loop: [`Socket::startServer`](srcs/Socket/Socket.hpp), [`Socket::acceptConnection`](srcs/Socket/Socket.hpp), [`Socket::receiveRequest`](srcs/Socket/Socket.hpp), [`Socket::sendResponse`](srcs/Socket/Socket.hpp)
- Request parsing: [`RequestParser::parse`](srcs/RequestParser/RequestParser.hpp)
- Response handling: [`Response::httpMethods`](srcs/Response/Response.hpp), [`Response::handleGET`](srcs/Response/Response.hpp), [`Response::handlePOST`](srcs/Response/Response.hpp)
- CGI handler: [`cgiHandler::postCgi`](srcs/cgi/cgiHandler.hpp), [`cgiHandler::configCgi`](srcs/cgi/cgiHandler.hpp), [`cgiHandler::createEnv`](srcs/cgi/cgiHandler.hpp)
- Config parsing: [`ConfigParser::initConfig`](srcs/ConfigParser/ConfigParser.hpp), [`ConfigParser::configServer`](srcs/ConfigParser/ConfigParser.hpp), [`ConfigParser::treatLocation`](srcs/ConfigParser/ConfigParser.hpp)
- Virtual server + locations: [`VirtualServer`](srcs/VirtualServer/VirtualServer.hpp), `struct Location` in the same header

## Features

- Single-process, event-driven server using poll()
- Multiple clients, non-blocking sockets
- Nginx-like configuration syntax
- Static files from configured roots
- Basic GET and POST handling
- Basic error statuses for 400/404/405 (static HTML error pages exist but are not yet wired to responses)
- Experimental CGI (Python) invocation on .py URIs

## Requirements

- Linux (uses poll, POSIX sockets, fcntl)
- g++ with C++98 support
- Python 3 at /usr/bin/python3 (used by CGI)
- Terminal (for build/run logs)

## Build

```sh
make
```

Outputs binary: ./webserver

Targets are defined in [Makefile](Makefile).

## Run

Run from the repository root (paths are relative).

- Default config:
```sh
./webserver
```

- Custom config:
```sh
./webserver ./conf/default.conf
```

The default config ([conf/default.conf](conf/default.conf)) listens on port 8080 and defines two servers (`server_name localhost` and `server_name 127.0.0.1`) on the same port. The active VirtualServer is selected by the Host header (without the port).

Examples:
- http://localhost:8080/ → serves [data/www/index.html](data/www/index.html)
- Host matching matters; e.g., curl with a specific Host:
  - `curl -H "Host: localhost" http://127.0.0.1:8080/`
  - `curl -H "Host: 127.0.0.1" http://127.0.0.1:8080/`

Note:
- Tilde paths are not expanded. The `root ~/data/www/` in the second `location` of the default config will not resolve unless changed to an absolute path.
- Stop with Ctrl+C (handled in [srcs/main.cpp](srcs/main.cpp)).

## Configuration

Syntax is parsed by [`ConfigParser`](srcs/ConfigParser/ConfigParser.hpp).

Example: [conf/default.conf](conf/default.conf)
```conf
server {
  listen 8080
  server_name localhost
  body_size 1K

  location / {
    root data/www/
    index index.html
  }

  location /data {
    # Tilde is not expanded by the server; use an absolute path instead
    root /absolute/path/to/data/www/
    index index.html
  }
}
```

Recognized directives in `server`:
- listen, server_name, body_size, error_page, location

Inside `location`, see `struct Location` in [`VirtualServer`](srcs/VirtualServer/VirtualServer.hpp):
- _locationPath, _root, _cgi_extension, _upload, _autoindex, _methods, _return, _index

Notes:
- Location matching is exact against the request URI (no prefix/longest-match logic).
- The selected VirtualServer is chosen by exact `Host` header match to `server_name` (port stripped).

## Request Handling

- Requests are parsed by [`RequestParser`](srcs/RequestParser/RequestParser.hpp):
  - Method, URI, version, headers, and body (for POST)
  - Host header parsing also extracts the port and stores Host without the port

- Responses are built by [`Response`](srcs/Response/Response.hpp):
  - GET: [`Response::handleGET`](srcs/Response/Response.hpp)
    - If `uri` matches a `location` exactly, serves `root + index[1]`
    - Else, for single-location configs, serves `root + uri`
    - On success returns 200 text/html with file contents
    - On miss returns 404 (body currently not using custom error page)
  - POST: [`Response::handlePOST`](srcs/Response/Response.hpp)
    - If URI ends with `.py`, invokes CGI via [`cgiHandler::postCgi`](srcs/cgi/cgiHandler.hpp)
    - Otherwise echoes a simple text/plain response if a body is present; else 400

- Static files are read in [`Response::readData`](srcs/Response/Response.hpp)

## CGI

- Entry script: [cgi-bin/index.py](cgi-bin/index.py)
- Trigger: POST to a URI ending with .py (e.g., /cgi-bin/index.py)
- Invoked by [`cgiHandler::postCgi`](srcs/cgi/cgiHandler.hpp) using `execve` and environment from [`cgiHandler::createEnv`](srcs/cgi/cgiHandler.hpp)

Current state:
- Experimental: child process executes Python, but the HTTP response body does not capture the script’s stdout back to the client. You will see a 200 OK with an empty body.
- The standalone test harness [`cgiHandler::configCgi`](srcs/cgi/cgiHandler.hpp) (used by [srcs/cgi/maincgi.cpp](srcs/cgi/maincgi.cpp)) demonstrates execution/redirection but is not wired into the main server path.

Ensure the script is executable. The form in [data/www/index.html](data/www/index.html) posts multipart/form-data to ../../cgi-bin/index.py.

## Static Content and Error Pages

- Main page: [data/www/index.html](data/www/index.html) (includes a form to POST to CGI)
- Alternate pages: [data/www/index_test.html](data/www/index_test.html), [data/www/alan_index.html](data/www/alan_index.html)
- Error page templates (not automatically returned yet):
  - 400: [data/error_pages/400.html](data/error_pages/400.html)
  - 404: [data/error_pages/404.html](data/error_pages/404.html)
  - 405: [data/error_pages/405.html](data/error_pages/405.html)

## Quick Test

- GET home:
```sh
curl -i http://localhost:8080/
```

- POST to CGI (multipart):
```sh
curl -i -F "nome=Alice" -F "email=alice@example.com" -F "imagem=@/path/to/image.png" http://localhost:8080/cgi-bin/index.py
```

- POST simple form data (echo path):
```sh
curl -i -d "hello=world" http://localhost:8080/
```

- Select a specific VirtualServer by Host:
```sh
curl -i -H "Host: localhost" http://127.0.0.1:8080/
```

## Project Structure

- Core
  - [srcs/main.cpp](srcs/main.cpp)
  - [`WebServer`](srcs/WebServer/WebServer.hpp), [`WebServer`](srcs/WebServer/WebServer.cpp)
  - [`Socket`](srcs/Socket/Socket.hpp), [`Socket`](srcs/Socket/Socket.cpp)
- HTTP
  - [`RequestParser`](srcs/RequestParser/RequestParser.hpp), [`RequestParser`](srcs/RequestParser/RequestParser.cpp)
  - [`Response`](srcs/Response/Response.hpp), [`Response`](srcs/Response/Response.cpp)
- Configuration
  - [`ConfigParser`](srcs/ConfigParser/ConfigParser.hpp), [`ConfigParser`](srcs/ConfigParser/ConfigParser.cpp)
  - [`VirtualServer`](srcs/VirtualServer/VirtualServer.hpp), [`VirtualServer`](srcs/VirtualServer/VirtualServer.cpp)
- Clients
  - [`Client`](srcs/Client/Client.hpp), [`Client`](srcs/Client/Client.cpp)
- CGI
  - [`cgiHandler`](srcs/cgi/cgiHandler.hpp), [`cgiHandler`](srcs/cgi/cgiHandler.cpp)
  - [cgi-bin/index.py](cgi-bin/index.py)
- Data
  - [data/www](data/www), [data/error_pages](data/error_pages)
- Includes
  - [includes/Includes.hpp](includes/Includes.hpp), [includes/Defines.hpp](includes/Defines.hpp), [includes/Classes.hpp](includes/Classes.hpp)

## Known Limitations

- Single listen socket; binds to the first configured VirtualServer port
- VirtualServer selection by exact Host header match only (no SNI, no IP-based)
- Location matching is exact against the request URI (no prefix/longest-match)
- Tilde (~) in paths is not expanded
- Some error responses currently miss body content
- No directory listing/autoindex
- No DELETE implementation yet
- CGI response body not returned to client (experimental plumbing)
- No TLS, no HTTP/1.1 persistent connections, no chunked encoding
- Minimal error handling and validation

## Development Notes

- Signals: SIGINT triggers clean exit in [srcs/main.cpp](srcs/main.cpp)
- Non-blocking clients: set via fcntl in [`Socket::acceptConnection`](srcs/Socket/Socket.hpp)
- Buffering: