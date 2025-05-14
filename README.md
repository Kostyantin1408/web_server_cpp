# **HTTP server with web sockets support**

In this project our intention is to create a simple HTTP server with web sockets support.   
  
> **Main requirements are**:
> - Support for HTTP requests and responses
> - Support for web sockets
> - Support for multiple clients
> - Support for multiple threads
> - Cross-platform compatibility
> - lightweight
> - intuitive API

## Prerequirements
- **g++** >= 14.2.0
- **cmake** >= 3.26

## Compilation
```bash
mkdir build
cd build
cmake ..
make
```

## Run example
```bash
./web_server
```

```shell
curl -X GET http://127.0.0.1:8080/ -H "Content-Type: application/json" -d '{"update":"new_value"}'
```

## Usage example
#### Start server
```c++
WebServer server{{"127.0.0.1", 8080}};
```
#### Add handler
```c++
server.get("/", [](int client_fd, const HttpRequest &req) {
    ...
    write(client_fd, response, std::strlen(response));
  });
```

## Third-pary libraries
```
https://github.com/Chocobo1/Hash
```

#### Stop server
```c++
server.request_stop();
```

