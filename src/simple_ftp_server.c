#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // used for close()
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <simple_ftp_server.h>
#include <helpers.h>

int serverSocket, clientSocket = -1;
FILE *file = NULL;

void gracefulExit()
{
    printf("Exiting program.\n");

    if (serverSocket >= 0)
    {
        printf("Closing server socket.\n");
        close(serverSocket);
    }

    if (clientSocket >= 0)
    {
        printf("Closing client socket.\n");
        close(clientSocket);
    }

    if (file != NULL)
    {
        printf("Closing file.\n");
        fclose(file);
    }
}

void signalHandler(int sig)
{
    exit(EXIT_SUCCESS);
}

void serverReply(int fd, FTP_replyCode_t replyCode)
{
    char replyStr[6];

    snprintf(replyStr, sizeof(replyStr), "%d\r\n", (int)replyCode);

    send(fd, replyStr, strlen(replyStr), 0);
}

void removeCommand(char *dataBuffer, int bufferSize)
{
    // assume commands are 4 characters
    for (unsigned int ii = 0; ii < 4; ii++) // set first 4 char to space
        dataBuffer[ii] = ' ';

    int offsetVal = 0;
    for (unsigned int ii = 0; ii < bufferSize; ii++) // set first 4 char to space
    {
        if (offsetVal == 0 && dataBuffer[ii] != ' ')
            offsetVal = ii;

        switch (dataBuffer[ii]) // remove new line and return
        {
        case '\r':
        case '\n':
            dataBuffer[ii] = '\0';
            break;
        }

        if (offsetVal)
        {
            dataBuffer[ii - offsetVal] = dataBuffer[ii];
        }
    }
}

FTP_clientRequest_t parseClientRequest(char *dataBuffer, int bufferSize)
{
    if (strncmp(dataBuffer, "RETR", min(4, bufferSize)) == 0) // retreive copy of file
    {
        removeCommand(dataBuffer, bufferSize);
        return FTP_clientRequest_retr;
    }
    else if (strncmp(dataBuffer, "USER", min(4, bufferSize)) == 0) // set user
    {
        removeCommand(dataBuffer, bufferSize);
        return FTP_clientRequest_user;
    }
    else if (strncmp(dataBuffer, "PASS", min(4, bufferSize)) == 0) // set pass
    {
        removeCommand(dataBuffer, bufferSize);
        return FTP_clientRequest_pass;
    }
    else if (strncmp(dataBuffer, "SYST", min(4, bufferSize)) == 0) // request system os
    {
        return FTP_clientRequest_syst;
    }
    else if (strncmp(dataBuffer, "QUIT", min(4, bufferSize)) == 0)
    {
        return FTP_clientRequest_quit;
    }

    return FTP_clientRequest_unknown;
}

void handleClient(int fd)
{
    // new user greeting message
    serverReply(fd, FTP_reply_serviceReadyForNewUser);

    // data buffer to read to
    char dataBuffer[DATA_BUFFER_SIZE];

    while (1)
    {
        // reset data buffer
        memset(dataBuffer, 0, DATA_BUFFER_SIZE);

        // read data
        int readBytes = recv(fd, dataBuffer, DATA_BUFFER_SIZE, 0);

        printf("read bytes: %d\n", readBytes);
        printf("%s\n", dataBuffer);

        if (readBytes < 1)
        {
            printf("failed to read bytes.\n");
            break;
        }

        FTP_clientRequest_t clientRequest = parseClientRequest(dataBuffer, DATA_BUFFER_SIZE);

        switch (clientRequest)
        {
        case FTP_clientRequest_user: // currently not checking user. here for testing.
            serverReply(fd, FTP_reply_sendPassword);
            break;
        case FTP_clientRequest_pass: // currently not checking pass. here for testing.
            serverReply(fd, FTP_reply_userLoggedOn);
            break;
        case FTP_clientRequest_syst: // request system os
            serverReply(fd, FTP_reply_osIsVm);
            break;
        case FTP_clientRequest_retr: // get copy of file
            printf("sending file %s\n", dataBuffer);
            // open file
            file = fopen(dataBuffer, "rb");

            if (file == NULL) // invalid file
            {
                printf("invalid file requested.\n");
                serverReply(fd, FTP_reply_reqNotTaken);
                break;
            }

            // send status start transfer
            serverReply(fd, FTP_reply_fileStatusOk);
            // send data in buffer sized chunks
            do
            {
                readBytes = fread(dataBuffer, sizeof(char), DATA_BUFFER_SIZE, file);
                send(fd, dataBuffer, readBytes, 0);
            } while (readBytes);

            if (fclose(file) == 0)
                file = NULL;

            // send status complete
            serverReply(fd, FTP_reply_closeDataConnection);

            break;
        case FTP_clientRequest_quit:
            serverReply(fd, FTP_reply_quit);
            goto closeClient;
            break;
        default:
            break;
        }
    }

closeClient:;

    close(clientSocket);
}

/**
 * This program creates a simple FTP server
 */
int main(int argc, char *argv[])
{
    // close sockets on graceful exit
    atexit(gracefulExit);

    // close sockets on segmentation fault
    signal(SIGSEGV, signalHandler);

    // close sockets on abort signal
    signal(SIGABRT, signalHandler);

    // close sockets on interactive attention
    signal(SIGINT, signalHandler);

    printf("Starting Simple FTP Server\n");

    // create server's socket
    printf("Creating socket... ");
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    // check for successful socket creation
    if (serverSocket < 0) // error
    {
        printf("Failed to create socket.\n");
        return EXIT_FAILURE;
    }
    printf("Successfully created socket.\n");

    // create internet socket address
    unsigned int port = FTP_PORT; // assigned to var to open potential for different port later
    struct sockaddr_in serverAddr =
        {
            .sin_addr = AF_INET,
            .sin_addr.s_addr = INADDR_ANY,
            .sin_port = htons(port)};

    // create client address struct
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    memset(&clientAddr, 0, sizeof(clientAddr)); // setting values to 0

    // set reuse addr option
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
    {
        printf("Failed to set SO_REUSEADDR option.");
        return EXIT_FAILURE;
    }

    // binding server socket and address
    printf("Binding name to socket... ");
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        printf("Failed to bind. sudo might be required or the port is in use.\n");
        return EXIT_FAILURE;
    }
    printf("Bind successful.\n");

    // listen for connection
    printf("Listening for client... ");
    if (listen(serverSocket, 5) < 0)
    {
        printf("Failed to listen.\n");
        return EXIT_FAILURE;
    }
    printf("Listen successful.\n");

    printf("FTP server started on port %d\n", port);

    while (1)
    {
        // accept incoming client
        printf("Accepting incoming client... ");
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (clientSocket < 0)
        {
            printf("Failed to accept client.\n");
            continue;
        }
        printf("Successfully accpeted client.\n");

        handleClient(clientSocket);
    };

    return EXIT_SUCCESS;
}