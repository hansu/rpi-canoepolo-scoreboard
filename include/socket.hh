#include <stdio.h>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>

class Socket
{
public:

    Socket(bool isServer) : m_isServer (isServer)
    {

    }

    // Create a Socket for server communication
    short SocketCreate(void)
    {
        printf("Create the socket\n");
        if(m_isServer){
            m_hServerSocket = socket(AF_INET, SOCK_STREAM, 0);
            return m_hServerSocket;
        }
        else{
            m_hSocket = socket(AF_INET, SOCK_STREAM, 0);
            return m_hSocket;
        }
    }

    /* Server only */
    int BindCreatedSocket(int ClientPort)
    {
        int iRetval = -1;
        struct sockaddr_in remote = {};
        /* Internet address family */
        remote.sin_family = AF_INET;
        /* Any incoming interface */
        remote.sin_addr.s_addr = htonl(INADDR_ANY);
        remote.sin_port = htons(ClientPort); /* Local port */
        iRetval = bind(m_hServerSocket, (struct sockaddr *)&remote, sizeof(remote));
        return iRetval;
    }

    /* Server only */
    int Accept(void){
        struct sockaddr_in client;
        int clientLen = sizeof(struct sockaddr_in);
        m_hSocket = accept(
            m_hServerSocket,(struct sockaddr *)&client,(socklen_t*)&clientLen);
        return m_hSocket;
    }

    /* Server only */
    int Listen(void)
    {
        return listen(m_hServerSocket, 3);
    }

    // connect to server
    int SocketConnect(std::string hostname, int ServerPort)
    {
        int iRetval = -1;
        struct sockaddr_in remote = {};
	    char ip[100];
	    hostname_to_ip(hostname.c_str() , ip);
        remote.sin_addr.s_addr = inet_addr(ip); //Local Host
        remote.sin_family = AF_INET;
        remote.sin_port = htons(ServerPort);
        iRetval = connect(
            m_hSocket, (struct sockaddr *)&remote, sizeof(struct sockaddr_in));
        return iRetval;
    }

    // Send the data to the server and set timeout to 20 seconds
    int SocketSend(std::string sRequest)
    {
        int shortRetval = -1;
        struct timeval tv;
        tv.tv_sec = 20; /* 20 Secs Timeout */
        tv.tv_usec = 0;
        if (setsockopt(m_hSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv)) < 0)
        {
            printf("Time Out\n");
            return -1;
        }
        shortRetval = send(m_hSocket, sRequest.c_str(), sRequest.length()+1, MSG_NOSIGNAL);
        return shortRetval;
    }
    // receive data from server
    int SocketReceive(std::string &sResponse)
    {
        int shortRetval = -1;
        struct timeval tv;
        tv.tv_sec = 20; /* 20 Secs Timeout */
        tv.tv_usec = 0;
        if (setsockopt(m_hSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)
        {
            printf("Time Out\n");
            return -1;
        }
        shortRetval = recv(m_hSocket, m_messageBuf, sizeof(m_messageBuf), MSG_NOSIGNAL);
        printf("Incoming: %s\n", m_messageBuf);
        sResponse = std::string((char*)m_messageBuf);
        return shortRetval;
    }

    int Close()
    {
        return close(m_hSocket);
    }

    int Shutdown(int how)
    {
        return shutdown(m_hSocket, how);
    }

private:
    bool m_isServer;
    int m_hSocket, m_hServerSocket;
    char m_messageBuf[200];

    int hostname_to_ip(const char * hostname , char* ip)
    {
        struct hostent *he;
        struct in_addr **addr_list;
        int i;

        if ( (he = gethostbyname( hostname ) ) == NULL)
        {
            // get the host info
            herror("gethostbyname");
            return 1;
        }

        addr_list = (struct in_addr **) he->h_addr_list;

        for(i = 0; addr_list[i] != NULL; i++)
        {
            // Return the first one
            strcpy(ip , inet_ntoa(*addr_list[i]) );
            return 0;
        }

        return 1;
    }

};
