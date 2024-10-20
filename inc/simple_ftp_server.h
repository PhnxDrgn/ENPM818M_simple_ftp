#define FTP_PORT 21
#define DATA_BUFFER_SIZE 1024

typedef enum
{
    FTP_reply_fileStatusOk = 150,
    FTP_reply_osIsVm = 215,
    FTP_reply_serviceReadyForNewUser = 220,
    FTP_reply_quit = 221,
    FTP_reply_closeDataConnection = 226,
    FTP_reply_userLoggedOn = 230,
    FTP_reply_sendPassword = 331,
    FTP_reply_reqNotTaken = 550,
} FTP_replyCode_t;

typedef enum
{
    FTP_clientRequest_retr,
    FTP_clientRequest_user,
    FTP_clientRequest_pass,
    FTP_clientRequest_quit,
    FTP_clientRequest_syst,
    FTP_clientRequest_unknown,
} FTP_clientRequest_t;