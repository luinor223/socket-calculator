#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <cstring>
using namespace std;

int main()
{
    int sockfd;
    int len;
    struct sockaddr_in address;
    int result;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = htons(9734);
    len = sizeof(address);

    result = connect(sockfd, (struct sockaddr *)&address, len);
    if (result < 0)
    {
        perror("An error occured while connecting");
        return 1;
    }
    cout << "Connection established." << endl;
    while (true)
    {
        cout << "Input an expression or 'L' to leave: ";
        string str;
        getline(cin, str);
        while (str.length() > 1023)
        {
            cout << "Expression too long, try again: ";
            getline(cin, str);
        }

        char buffer[1024];
        strcpy(buffer, str.c_str());

        result = write(sockfd, buffer, 1024);
        if (result < 0)
        {
            close(sockfd);
            perror("An error occured while writing");
            return 1;
        }

        if (str == "L")
        {
            close(sockfd);
            break;
        }

        char answer[1024] = {0};
        result = read(sockfd, answer, 1024);
        if (result < 0)
        {
            close(sockfd);
            perror("An error occured while reading");
            return 1;
        }
        cout << "Result received from server: " << answer << endl;
    }
    close(sockfd);

    return 0;
}