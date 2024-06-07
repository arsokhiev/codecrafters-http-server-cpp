#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <list>
#include <thread>

std::list<int> g_connections;

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

void read_user_agent(std::string& user_agent_value, const std::string& string_request)
{
    size_t user_agent_pos= string_request.find("User-Agent: ");
    size_t user_agent_end_pos = user_agent_pos + 12;

    if (user_agent_pos != std::string::npos)
    {
        size_t start = user_agent_end_pos;
        size_t end = string_request.find('\r', start);

        if (end != std::string::npos)
        {
            user_agent_value = string_request.substr(start, end - start);
        }
    }
}

void make_response(std::string& response_message, const std::string& path, const std::string& user_agent_value)
{
    if (path == "/")
    {
        response_message = "HTTP/1.1 200 OK\r\n\r\n";
    }
    else if (path.find("/echo/") == 0)
    {
        std::string content = path.substr(6);
        response_message =
                // Status line
                "HTTP/1.1 200 OK"
                "\r\n"
                // Headers
                "Content-Type: text/plain\r\n"
                "Content-Length: "
                + std::to_string(content.size()) + "\r\n\r\n" + content;
    }
    else if (!user_agent_value.empty())
    {
        response_message =
                // Status line
                "HTTP/1.1 200 OK"
                "\r\n"
                // Headers
                "Content-Type: text/plain\r\n"
                "Content-Length: "
                + std::to_string(user_agent_value.size()) + "\r\n\r\n"
                // Response body
                + user_agent_value;
    }
    else
    {
        response_message = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    }
}

ssize_t send(int client_fd, const std::string& response_message)
{
    return send(client_fd, response_message.c_str(), response_message.length(), 0);
}

void add_connection(int client_fd)
{
    g_connections.push_back(client_fd);
}

void client_handler(int client_fd)
{
    std::string string_request(1024, '\0');
    std::string path, user_agent_value, response_message;

    if (receive(client_fd, string_request) == -1)
    {
        std::cerr << "Error receiving message from client\n";
        close(client_fd);
        return;
    }

    find_path_from_request(path, string_request);
    read_user_agent(user_agent_value, string_request);

    make_response(response_message, path, user_agent_value);
    //std::cout << "\n\n\nresponse_message:\n" << response_message << "\n\n\n";

    send(client_fd, response_message);

    close(client_fd);
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

    while (true)
    {
	    std::cout << "Waiting for a client to connect...\n";

	    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, (socklen_t*)&client_addr_len);
        std::cout << "Client connected\n";

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        client_handler(client_fd);
    }

    close(server_fd);
	return 0;
}
