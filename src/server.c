/**
 * webserver.c -- A webserver written in C
 * 
 * Test with curl (if you don't have it, install it):
 * 
 *    curl -D - http://localhost:3490/
 *    curl -D - http://localhost:3490/d20
 *    curl -D - http://localhost:3490/date
 * 
 * You can also test the above URLs in your browser! They should work!
 * 
 * Posting Data:
 * 
 *    curl -D - -X POST -H 'Content-Type: text/plain' -d 'Hello, sample data!' http://localhost:3490/save
 * 
 * (Posting data is harder to test from a browser.)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/file.h>
#include <fcntl.h>
#include "net.h"
#include "file.h"
#include "mime.h"
#include "cache.h"
#include <stdbool.h>

#define PORT "3490" // the port users will be connecting to

#define SERVER_FILES "./serverfiles"
#define SERVER_ROOT "./serverroot"

/**
 * Send an HTTP response
 *
 * header:       "HTTP/1.1 404 NOT FOUND" or "HTTP/1.1 200 OK", etc.
 * content_type: "text/plain", etc.
 * body:         the data to send.
 * 
 * Return the value from the send() function.
 */
int send_response(int fd, char *header, char *content_type, void *body, int content_length)
{
    const int max_response_size = 65536;
    char response[max_response_size];

    // Build HTTP response and store it in response

    ///////////////////
    // IMPLEMENT ME! //
    ///////////////////
    time_t rawtime = time(NULL);
    struct tm *timeinfo;
    timeinfo = localtime(&rawtime);
    // char date= asctime(timeinfo);
    char connection[] = "close";

    int response_length = sprintf(response, "%s\n"
                                            "Date: %s"
                                            "Connection: %s\n"
                                            "Content Length: %d\n"
                                            "Content Type: %s\n"
                                            "\n",
                                  header, asctime(timeinfo), connection, content_length, content_type);

    memcpy(response + response_length, body, content_length);

    // Send it all!
    int rv = send(fd, response, response_length + content_length, 0);

    if (rv < 0)
    {
        perror("send");
    }

    return rv;
}

/**
 * Send a /d20 endpoint response
 */
void get_d20(int fd)
{
    // Generate a random number between 1 and 20 inclusive

    ///////////////////
    // IMPLEMENT ME! //
    ///////////////////
    srand(time(0)); // will allow for random set instead of the same numbers each time.
    char body[8];
    sprintf(body, "%d\n", (rand() % 20) + 1);
    // Use send_response() to send it back as text/plain data
    send_response(fd, "HTTP/1.1 200 OK", "text/plain", body, strlen(body));
    ///////////////////
    // IMPLEMENT ME! //
    ///////////////////
}

/**
 * Send a 404 response
 */
void resp_404(int fd)
{
    char filepath[4096];
    struct file_data *filedata;
    char *mime_type;

    // Fetch the 404.html file
    snprintf(filepath, sizeof filepath, "%s/404.html", SERVER_FILES);
    filedata = file_load(filepath);

    if (filedata == NULL)
    {
        // TODO: make this non-fatal
        fprintf(stderr, "cannot find system 404 file\n");
        exit(3);
    }

    mime_type = mime_type_get(filepath);

    send_response(fd, "HTTP/1.1 404 NOT FOUND", mime_type, filedata->data, filedata->size);

    file_free(filedata);
}

/**
 * Read and return a file from disk or cache
 */
bool cacheable(int fd, struct cache *cache, char *file_path)
{
    struct cache_entry *entry;

    entry = cache_get(cache, file_path);

    if (entry != NULL)
    {
        //returns NULL IF IT DOESN'T exist.
        send_response(fd, "HTTP/1.1 200 OK", entry->content_type, entry->content, entry->content_length);
        return true;
    }
    else
    {
        return false;
    }
}
void get_file(int fd, struct cache *cache, char *request_path)
{
    ///////////////////
    // IMPLEMENT ME! //
    ///////////////////
    char file_path[5000];
    char *type;

    bool in_cache;

    printf("request path %s\n", request_path);
    // printf("%d, %s, %d\n", fd, request_path, cache->cur_size);
    // printf("%d, %s, %d\n", fd, request_path);
    //just to remove the warnings to start server and test my code in other areas.
    // resp_404(fd);
    snprintf(file_path, sizeof(file_path), "%s%s", SERVER_ROOT, request_path);
    //When a file is requested, first check to see if the path to the file is in the cache (use the file path as the key).
    in_cache = cacheable(fd, cache, file_path); //added in for cache.
    //If it's not there:
    if (!in_cache)
    {

        struct file_data *file_data_actual;

        file_data_actual = file_load(file_path);

        if (file_data_actual == NULL)
        {
            resp_404(fd);
            return;
        }

        type = mime_type_get(file_path);
        // printf("file path %s\n", file_path);

        send_response(fd, "HTTP/1.1 200 OK", type, file_data_actual->data, file_data_actual->size);

        //store it in the cache required 
        cache_put(cache, file_path, type, file_data_actual->data, file_data_actual->size); 
        //free the struct
        file_free(file_data_actual);
    }
}

/**
 * Search for the end of the HTTP header
 * 
 * "Newlines" in HTTP can be \r\n (carriage return followed by newline) or \n
 * (newline) or \r (carriage return).
 */
char *find_start_of_body(char *header)
{
    ///////////////////
    // IMPLEMENT ME! // (Stretch)
    ///////////////////

    return header; //just doing this to get rid of warning for now.
}

/**
 * Handle HTTP request and send response
 */
void handle_http_request(int fd, struct cache *cache)
{
    const int request_buffer_size = 65536; // 64K
    char request[request_buffer_size];

    char type_of_request[8];
    char path_of_request[1024];
    char protocol[128];

    // Read request
    int bytes_recvd = recv(fd, request, request_buffer_size - 1, 0);

    if (bytes_recvd < 0)
    {
        perror("recv");
        return;
    }

    request[bytes_recvd] = '\0'; //terminate added.

    ///////////////////
    // IMPLEMENT ME! //
    ///////////////////

    // Read the three components of the first request line
    sscanf(request, "%s %s %s", type_of_request, path_of_request, protocol);
    //sscanf reads formated input from a string so it should grap the input from request and set it to the others.
    printf("%s %s %s\n", type_of_request, path_of_request, protocol);
    //sscanf is doing as it should  printing out  GET  /d20   and HTTP/1.1

    //check it out
    printf("Requesting: %s %s %s\n", type_of_request, path_of_request, protocol);

    // If GET, handle the get endpoints
    if (strcmp(type_of_request, "GET") == 0)
    {
        //    Check if it's /d20 and handle that special case
        //    Otherwise serve the requested file by calling get_file()
        if (strcmp(path_of_request, "/d20") == 0)
        {
            get_d20(fd);
        }
        else
        {
            get_file(fd, cache, path_of_request);
        }
    }
    else
    {
        resp_404(fd);
    }

    // (Stretch) If POST, handle the post request
}

/**
 * Main
 */
int main(void)
{
    int newfd;                          // listen on sock_fd, new connection on newfd
    struct sockaddr_storage their_addr; // connector's address information
    char s[INET6_ADDRSTRLEN];

    struct cache *cache = cache_create(10, 0);

    // Get a listening socket
    int listenfd = get_listener_socket(PORT);

    if (listenfd < 0)
    {
        fprintf(stderr, "webserver: fatal error getting listening socket\n");
        exit(1);
    }

    printf("webserver: waiting for connections on port %s...\n", PORT);

    // This is the main loop that accepts incoming connections and
    // forks a handler process to take care of it. The main parent
    // process then goes back to waiting for new connections.

    while (1)
    {
        socklen_t sin_size = sizeof their_addr;

        // Parent process will block on the accept() call until someone
        // makes a new connection:
        newfd = accept(listenfd, (struct sockaddr *)&their_addr, &sin_size);
        if (newfd == -1)
        {
            perror("accept");
            continue;
        }

        // Print out a message that we got the connection
        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  s, sizeof s);
        printf("server: got connection from %s\n", s);

        // newfd is a new socket descriptor for the new connection.
        // listenfd is still listening for new connections.

        handle_http_request(newfd, cache);

        close(newfd);
    }

    // Unreachable code

    return 0;
}
