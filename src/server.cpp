#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

ssize_t receive(int client_fd, std::string& return_client_command)
{
    return recv(client_fd, (void*)&return_client_command[0], return_client_command.size(), 0);
}

void find_path_from_request(std::string& path, const std::string& string_request)
{
    size_t method_end = string_request.find(' ');
    if (method_end != std::string::npos)
    {
        size_t start = method_end + 1;
        size_t end = string_request.find(' ', start);

        if (end != std::string::npos)
        {
            path = string_request.substr(start, end - start);
        }
    }
}

ssize_t send(int client_fd, const std::string& response_message)
{
    return send(client_fd, response_message.c_str(), response_message.length(), 0);
}

void make_response(std::string& response_message, const std::string& path)
{
    if (path == "/")
    {
        response_message = "HTTP/1.1 200 OK\r\n\r\n";
    }
    else if(path.find("/echo/") == 0)
    {
        std::string content = path.substr(6);
        response_message =
                // Status line
                "HTTP/1.1 200 OK"
                "\r\n"
                // Headers
                "Content-Type: text/plain\r\n"
                "Content-Length: "
                + std::to_string(content.size()) + "\r\n\r\n" + content
                // Response body
                + ;
    }
    else
    {
        response_message = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    }
}

int main(int argc, char** argv)
{
	// Flush after every std::cout / std::cerr
	std::cout << std::unitbuf;
	std::cerr << std::unitbuf;

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0)
    {
		std::cerr << "Failed to create server socket\n";
		return 1;
	}

	// Since the tester restarts your program quite often, setting SO_REUSEADDR
	// ensures that we don't run into 'Address already in use' errors
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
		std::cerr << "setsockopt failed\n";
		return 1;
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(4221);

	if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0)
    {
		std::cerr << "Failed to bind to port 4221\n";
		return 1;
	}

	int connection_backlog = 5;
	if (listen(server_fd, connection_backlog) != 0)
    {
		std::cerr << "listen failed\n";
		return 1;
	}

	struct sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);

	std::cout << "Waiting for a client to connect...\n";

	int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, (socklen_t*)&client_addr_len);
    std::cout << "Client connected\n";

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    std::string string_request(1024, '\0');
    std::string path, response_message;

    if (receive(client_fd, string_request) == -1)
    {
        std::cerr << "Error receiving message from client\n";
        close(server_fd);
        close(client_fd);
        return 1;
    }

    find_path_from_request(path, string_request);
    make_response(response_message, path);

    send(client_fd, response_message);

	close(server_fd);
    close(client_fd);
	return 0;
}
