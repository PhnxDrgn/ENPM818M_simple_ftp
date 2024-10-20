#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // used for close()
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <arpa/inet.h>
#include <simple_ftp_server.h>
#include <helpers.h>

int clientSocket = -1;
FILE *file = NULL;

void gracefulClose()
{
    printf("Exiting program.\n");

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

void sendCommand(int fd, const char *cmd)
{
    send(fd, cmd, strlen(cmd), 0);
}

/**
 * get reply code by reading first 3 char as a number
 */
int getReplyCode(int fd, char *dataBuffer, int bufferSize)
{
    // clear buffer
    memset(dataBuffer, 0, bufferSize);

    // read response
    int bytesRead = recv(fd, dataBuffer, DATA_BUFFER_SIZE - 1, 0);
    dataBuffer[bytesRead] = '\0'; // terminate buffer with null

    if (bytesRead > 0)
    {
        char replyCode[4] = {0, 0, 0, 0};
        strncpy(replyCode, dataBuffer, min(3, bufferSize));

        int replyCodeInt = atoi(replyCode);

        if (replyCodeInt == 0)
            return -1;

        return replyCodeInt;
    }

    return -1;
}

void writeDataToFile(int fd, char *fileName)
{
    char dataBuffer[DATA_BUFFER_SIZE];

    int bytesRead = 0;

    file = fopen(fileName, "wb");

    if (file == NULL)
    {
        printf("File failed to open.\n");
        exit(EXIT_FAILURE);
    }

    do
    {
        // read in data
        bytesRead = recv(fd, dataBuffer, DATA_BUFFER_SIZE, 0);

        // find close data connection code
        char *endPtr = strstr(dataBuffer, "226\r\n");

        if (endPtr != NULL) // found end pointer
        {
            int lastSegLen = endPtr - dataBuffer;

            fwrite(dataBuffer, sizeof(dataBuffer[0]), lastSegLen, file);

            if (fclose(file) == 0)
                file = NULL;

            return;
        }

        fwrite(dataBuffer, sizeof(dataBuffer[0]), DATA_BUFFER_SIZE, file);

    } while (bytesRead);
}

/**
 * This program creates a simple FTP server
 */
int main(int argc, char *argv[])
{
    // close sockets on graceful exit
    atexit(gracefulClose);

    // close sockets on segmentation fault
    signal(SIGSEGV, signalHandler);

    // close sockets on abort signal
    signal(SIGABRT, signalHandler);

    // close sockets on interactive attention
    signal(SIGINT, signalHandler);

    // create string for server ip
    char serverIp[16] = "\0";  // assume IPv4 so max len is 255.255.255.255
    char srcFile[255] = "\0";  // file path on server
    char destFile[255] = "\0"; // file path on client

    if (argc < 4) // expect arg to be server ip, source file name, dest file name
    {
        printf("3 args required. ip, src, dest.\n");
        exit(EXIT_FAILURE);
    }

    strncpy(serverIp, argv[1], sizeof(serverIp));
    strncpy(srcFile, argv[2], sizeof(srcFile));
    strncpy(destFile, argv[3], sizeof(destFile));

    printf("Starting Simple FTP Client\n");

    // create client's socket
    printf("Creating socket... ");
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    // check for successful socket creation
    if (clientSocket < 0) // error
    {
        printf("Failed to create socket.\n");
        return EXIT_FAILURE;
    }
    printf("Successfully created socket.\n");

    // create server addr
    struct sockaddr_in serverAddr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = inet_addr(serverIp),
        .sin_port = htons(FTP_PORT)};

    printf("Attempting to connect to FTP server (%s)... ", serverIp);
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) // failed to connect
    {
        printf("Failed to connect.\n");
        exit(EXIT_FAILURE);
    }
    printf("Connected to FTP server.\n");

    int replyCode = 0;
    char dataBuffer[DATA_BUFFER_SIZE];
    replyCode = getReplyCode(clientSocket, dataBuffer, DATA_BUFFER_SIZE);

    printf("Received reply: %d\n", replyCode);

    if (replyCode != (int)FTP_reply_serviceReadyForNewUser) // expect 220 as reply
    {
        printf("Did not receive reply %d\n", (int)FTP_reply_serviceReadyForNewUser);
        return EXIT_FAILURE;
    }

    // send file request
    snprintf(dataBuffer, DATA_BUFFER_SIZE, "RETR %s\r\n", srcFile);
    sendCommand(clientSocket, dataBuffer);

    // check for start file send message
    replyCode = getReplyCode(clientSocket, dataBuffer, DATA_BUFFER_SIZE);
    printf("Received reply: %d\n", replyCode);
    if (replyCode != (int)FTP_reply_fileStatusOk) // expect file status ok
    {
        printf("Did not receive reply %d\n", (int)FTP_reply_fileStatusOk);
        return EXIT_FAILURE;
    }

    writeDataToFile(clientSocket, destFile);

    printf("Received reply: %d\n", replyCode);

    sendCommand(clientSocket, "QUIT");

    return EXIT_SUCCESS;
}