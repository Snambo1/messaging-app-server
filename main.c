#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 512

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <process.h>

FILE *users;//user database file for account making and logging in
FILE *usernames;//list of taken usernames

struct user {
    SOCKET connection;//socket user is connected to
    char username[20];//their username
};

struct user *connecters;//dynamic list of connected users
int length=0;//length of connected users

struct user* remove_element(struct user* array, int sizeOfArray, int indexToRemove){
    struct user* temp = malloc((sizeOfArray - 1) * sizeof(struct user)); // allocate an array with a size 1 less than the current one

    if (indexToRemove != 0)
        memcpy(temp, array, indexToRemove * sizeof(struct user)); // copy everything BEFORE the index

    if (indexToRemove != (sizeOfArray - 1))
        memcpy(temp+indexToRemove, array+indexToRemove+1, (sizeOfArray - indexToRemove - 1) * sizeof(struct user)); // copy everything AFTER the index

    free (array);
    return temp;
}

unsigned __stdcall ClientSession(void *data){//multithreading, gonna  be real  with you I have no idea how this code works
    SOCKET ClientSocket = (SOCKET)data;// Process the client.

    //message receiving setup stuff
    char message[DEFAULT_BUFLEN];
    int buffer;//function recorder
    int buffer2;//another function  recorder
    int iSendResult;
    int messageSize = DEFAULT_BUFLEN;
    char sender[100];//user that  is sending stuff

    //login shit
    bool logger=false;
    char accounter[1];//records if  they have an account

    do {//it has to be a do while so the thread ends properly
        buffer2=recv(ClientSocket, accounter, 1, 0);//checking if they need to make a new account or can log in
        printf("%d\n",buffer2);
        if (!(buffer2>=0)){
            printf("recv failed: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            return 1;
        }

        if (accounter[0]=='N'){
            users=fopen("users.txt","a");//opens the user recording file
            char holder[2][20];//holds the login info
            while (true){
                usernames=fopen("usernames.txt","r");//opens the username list file
                bool taken=false;
                buffer2=recv(ClientSocket, holder[0], 20, 0);//getting the username
                if (!(buffer2>=0)){
                    printf("recv failed: %d\n", WSAGetLastError());
                    closesocket(ClientSocket);
                    return 1;
                }

                if (strcmp(holder[0]," ")==0){//checks if it's just a timeout space
                    memset(holder[0], 0, sizeof(holder[0]));//resetting login info if it's just a space
                    continue;
                }

                //checks if the username is taken
                char checker[100];
                while(fgets(checker, 100, usernames)) {
                    if (checker[strlen(checker)-1]== '\n') checker[strlen(checker)-1]='\0';
                    if (strcmp(checker,holder[0])==0){
                        taken=true;
                        char denier[]="taken";
                        send(ClientSocket, denier, (int)strlen(denier), 0);
                        break;
                    }
                }
                fclose(usernames);
                if (taken) continue;
                char accepter[]="okie-dokie";
                send(ClientSocket, accepter, (int)strlen(accepter), 0);
                break;
            }

            while (true){
                buffer2=recv(ClientSocket, holder[1], 20, 0);//getting the login info
                if (!(buffer2>=0)){
                    printf("recv failed: %d\n", WSAGetLastError());
                    closesocket(ClientSocket);
                    return 1;
                }

                if (strcmp(holder[1]," ")==0){
                    memset(holder[1], 0, sizeof(holder[1]));//resetting login info if it's just a space
                    continue;
                }
                break;
            }
            //recording the new account
            char account[] = " username: ";
            strcat(account,holder[0]);
            strcat(account," password: ");
            strcat(account,holder[1]);
            strcat(account,"\n");
            fprintf(users,account);//recording  login info
            strcat(holder[0],"\n");
            usernames=fopen("usernames.txt","a");
            fprintf(usernames,holder[0]);//recording the username
            fclose(users);
            fclose(usernames);
        }

        if (accounter[0]=='Y'){
            char holder[2][20];//stores login info
            while (true){
                buffer2=recv(ClientSocket, holder[0], 20, 0);//getting the login info
                if (!(buffer2>=0)){
                    printf("recv failed: %d\n", WSAGetLastError());
                    closesocket(ClientSocket);
                    return 1;
                }

                if (strcmp(holder[0]," ")==0){
                    memset(holder[0], 0, sizeof(holder[0]));//resetting login info if it's just a space
                    continue;
                }
                break;
            }
            while (true){
                buffer2=recv(ClientSocket, holder[1], 20, 0);//getting the login info
                if (!(buffer2>=0)){
                    printf("recv failed: %d\n", WSAGetLastError());
                    closesocket(ClientSocket);
                    return 1;
                }

                if (strcmp(holder[1]," ")==0){
                    memset(holder[1], 0, sizeof(holder[1]));//resetting login info if it's just a space
                    continue;
                }
                break;
            }

            char account[] = " username: ";
            strcat(account,holder[0]);
            strcat(account," password: ");
            strcat(account,holder[1]);
            printf("%s\n",account);

            char accountBuffer[100];
            bool noper=false;
            users=fopen("users.txt","r");
            while (fgets(accountBuffer, 100, users)){
                if (accountBuffer[strlen(accountBuffer)-1]== '\n') accountBuffer[strlen(accountBuffer)-1]='\0';//checks if last character is newline
                if (strcmp(account,accountBuffer)==0){
                    //login confirmation stuff
                    logger=true;
                    send(ClientSocket, "Y", 1, 0);//yes they logged in right
                    printf("Logged in\n");
                    length++;
                    connecters=realloc(connecters,length*sizeof(struct user));
                    struct user newGuy = {
                        .connection=ClientSocket
                    };
                    memcpy(newGuy.username, holder[0], 20);
                    connecters[length-1]=newGuy;
                    fclose(users);
                    strcpy(sender,holder[0]);
                    noper=true;
                    break;
                }
            }

            if (!noper) send(ClientSocket, "N", 1, 0);//no they logged in wrong
            memset(holder[0], 0, sizeof(holder[0]));//resetting the login info
            memset(holder[1], 0, sizeof(holder[1]));//resetting the login info
        }
    } while (!logger&&buffer2>0);//making it so the thread ends properly if connection is insantly abandoned

    // Receive until the peer shuts down the connection
    do {
        memset(message, 0, sizeof(message));//resetting the message
        buffer = recv(ClientSocket, message, messageSize, 0);//receiving  the message
        if (buffer > 0) {
            if (strcmp(message," ")==0) continue;
            printf("%s\n",message);//prints message they sent
            char receiver[50];
            memset(receiver, 0, sizeof(receiver));//resetting this because for some reason it doesn't work when I don't
            while (true){
                buffer2=recv(ClientSocket, receiver, 20, 0);//receiving  the receiver
                if (!(buffer2>=0)){
                    printf("recv failed: %d\n", WSAGetLastError());
                    closesocket(ClientSocket);
                    return 1;
                }
                if (strcmp(receiver," ")==0){
                    memset(receiver, 0, sizeof(receiver));//resetting the receiver if it's just a space
                    continue;
                }
                break;
            }
            //searches for user to send the message to
            for (int i=0;i<length;i++){
                if (strcmp(connecters[i].username,receiver)==0){
                    iSendResult = send(connecters[i].connection, sender, sizeof(sender), 0);
                    if (iSendResult == SOCKET_ERROR) {
                        printf("send failed: %d\n", WSAGetLastError());
                        closesocket(ClientSocket);
                        return 1;
                    }
                    //send  message to correct person
                    iSendResult = send(connecters[i].connection, message, sizeof(message), 0);
                    if (iSendResult == SOCKET_ERROR) {
                        printf("send failed: %d\n", WSAGetLastError());
                        closesocket(ClientSocket);
                        return 1;
                    }
                }
            }
        } else if (buffer == 0)
            printf("Connection closing...\n");
        else {
            printf("recv failed: %d\n", WSAGetLastError());
            for (int i=0;i<length;i++){
                if (connecters[i].connection==ClientSocket&&strcmp(connecters[i].username,sender)==0){
                    connecters = remove_element(connecters, length, i);
                    length--;
                }
            }
            closesocket(ClientSocket);
            return 1;
        }

    } while (buffer > 0);

    //removing them from the list
    for (int i=0;i<length;i++){
        if (connecters[i].connection==ClientSocket&&strcmp(connecters[i].username,sender)==0){
            connecters = remove_element(connecters, length, i);
            length--;
        }
    }
    closesocket(ClientSocket);
    printf("thread ended\n");
    return 0;
}

int main(){
    connecters=(struct user*)malloc(length * sizeof(struct user));//list of connected users

    //winsock setup variables
    WSADATA wsaData;
    int buffer;
    struct sockaddr_in service;//struct that stores ip address

    // Initialize Winsock
    buffer = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (buffer != 0) {
        printf("WSAStartup failed: %d\n", buffer);
        return 1;
    }

    // Create a SOCKET for listening for
    // incoming connection requests
    SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ListenSocket == INVALID_SOCKET) {
        wprintf(L"socket function failed with error: %u\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // The sockaddr_in structure specifies the address family,
    // IP address, and port for the socket that is being bound.
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = inet_addr("10.0.0.104");//ip to bind to
    service.sin_port = htons(27015);//port


    // Bind the socket.
    buffer = bind(ListenSocket, (SOCKADDR *) &service, sizeof (service));
    if (buffer == SOCKET_ERROR) {
        wprintf(L"bind failed with error %u\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    //listening for incoming socket
    while (true){
        if ( listen( ListenSocket, SOMAXCONN ) == SOCKET_ERROR ) {
            printf( "Listen failed with error: %d\n", WSAGetLastError() );
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }
        SOCKET ClientSocket;
        ClientSocket = INVALID_SOCKET;

        // Accept a client socket
        ClientSocket = accept(ListenSocket, NULL, NULL);
        printf("%llu\n",ClientSocket);//printing the client socket
        if (ClientSocket == INVALID_SOCKET) {
            printf("accept failed: %d\n", WSAGetLastError());
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }
        // Create a new thread for the accepted client (also pass the accepted client socket).
        unsigned threadID;
        (HANDLE)_beginthreadex(NULL, 0, &ClientSession, (void*)ClientSocket, 0, &threadID);
    }

    // cleanup
    WSACleanup();
    printf("socket adventures");
    return 0;
}
