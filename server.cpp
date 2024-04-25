/* 1.Tạo các #include cần thiết */
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stack>
#include <string>
#include <vector>
/* dành riêng cho AF_INET */
#include <netinet/in.h>
#include <arpa/inet.h>
using namespace std;
vector<pair<int, pthread_t>> threads;

bool parse(string& tokens)
{
    //Remove spaces
    string temp = "";
    for (int i = 0; i < tokens.size(); i++)
    {
        if (tokens[i] == ' ') continue;
        temp += tokens[i];
    }
    tokens = temp;
    temp = "";

    //Parse
    int brackets_count = 0;
    for (int i = 0; i < tokens.size(); i++)
    {
        //Check for brackets, add * if needed
        if (tokens[i] == '(')
        {
            brackets_count++;
            if (string("*/)").find(tokens[i+1]) != string::npos)
                return false;
                
            if (i > 0 && (isdigit(tokens[i-1]) || tokens[i-1] == ')'))
                temp += '*';
            temp += tokens[i];
        }
        else if (tokens[i] == ')')
        {
            brackets_count--;
            if (brackets_count < 0)
                return false;
            temp += tokens[i];
            if (i < tokens.size() - 1 && isdigit(tokens[i+1]))
                temp += '*';
        }
        //Check for multiple consecutive signs
        else if (string("+-").find(tokens[i]) != string::npos)
        {
            if (i == tokens.size() - 1) return false;
            int sign = 1;
            while (i < tokens.size() && string("+-").find(tokens[i]) != string::npos)
            {
                sign *= (int)(','-tokens[i]);
                i++;
            }
            i--;
            temp += sign > 0 ? '+' : '-';
        }
        else if (string("*/").find(tokens[i]) != string::npos)
        {
            if (i == 0 || i == tokens.size() - 1)
                return false;
            if (string("*/").find(tokens[i+1]) != string::npos)
                return false;
            temp += tokens[i];
        }
        else if (isdigit(tokens[i]))
            temp += tokens[i];
        else
            return false;
    }
    if (brackets_count != 0)
        return false;
    tokens = temp;
    temp = "";

    for (int i = 0; i < tokens.size(); i++)
    {
        if (tokens[i] == '+' && (i == 0 || string("*/(").find(tokens[i-1]) != string::npos))
            continue;
        if (tokens[i] == '-' && (i == 0 || string("*/(").find(tokens[i-1]) != string::npos))
        {
            temp += 'u';
            continue;
        }
        temp += tokens[i];
    }
    tokens = temp;
    return true;

}

int precedence(char c)
{
    if (c == '+' || c == '-')
        return 1;
    if (c == '*' || c == '/')
        return 2;
    if (c == 'u')
        return 3;
    
    return 0;
}

string InfixToRpn(string tokens)
{
    string output = "";
    stack<char> op;
    for (int i = 0; i < tokens.size(); i++)
    {
        
        if (isdigit(tokens[i]))
        {
            while (isdigit(tokens[i]))
            {
                output += tokens[i];
                i++;
            }
            output += ' ';
            i--;
        }
        else if (tokens[i] == '(')
            op.push(tokens[i]);
        else if (tokens[i] == ')')
        {
            while (op.top() != '(')
            {
                output += op.top();
                output += ' ';
                op.pop();
            }
            op.pop();
        }
        else
        {
            while (!op.empty() && precedence(tokens[i]) <= precedence(op.top()))
            {
                
                output += op.top();
                output += ' ';
                op.pop();
            }
            op.push(tokens[i]);
        }
    }
    while (!op.empty())
    {
        output += op.top();
        output += ' ';
        op.pop();
    }
    return output.substr(0, output.size() - 1);
}

double evalRpn(string tokens)
{
    stack <double> result;
    for (int i = 0; i < tokens.size(); i++)
    {
        if (tokens[i] == ' ')
            continue;
        else if (isdigit(tokens[i]))
        {
            int temp = 0;
            while (isdigit(tokens[i]))
            {
                temp = temp * 10 + (tokens[i] - '0');
                i++;
            }
            i--;
            result.push(temp);
        }
        else if (tokens[i] == 'u')
        {
            double a = result.top();
            result.pop();
            result.push(-a);
        }
        else
        {
            double b = result.top();
            result.pop();
            double a = result.top();
            result.pop();
            if (tokens[i] == '+')
                result.push(a + b);
            else if (tokens[i] == '-')
                result.push(a - b);
            else if (tokens[i] == '*')
                result.push(a * b);
            else if (tokens[i] == '/')
            {
                if (b == 0)
                    throw invalid_argument("Divide by zero");
                result.push(a / b);
            }
        }
    }
    return result.top();
}

void *connection_handler(void *p_client_sock_fd)
{
    int client_sockfd = *(int*)p_client_sock_fd, test;
    char buffer[1024] = { 0 };
    string msg;

    while (1) {
        test = read(client_sockfd, buffer, 1024);
        if (test == -1)
        {
            cout << "An error occured while reading" << endl;
            continue;
        }
        cout << "Received expression from client " << client_sockfd << ": " << buffer << endl;

        if (strcmp(buffer, "L") == 0) {
            cout << "Client number " << client_sockfd << " has left.\n";
            break;
        }
        
        string tokens = buffer;

        if (!parse(tokens)) {
            msg = "Syntax error";
        }
        else try {
            int result = evalRpn(InfixToRpn(tokens));
            msg = to_string(result);
        }
        catch (const invalid_argument& e) {
            msg = e.what();
        }
        
        strcpy(buffer, msg.c_str());
        test = write(client_sockfd, buffer, strlen(buffer));
        if (test == -1)
        {
            cout << "An error occured while writing" << endl;
            continue;
        }
        cout << "Message sent to client " << client_sockfd << ": " << buffer << endl;
    }

    for (vector<pair<int, pthread_t>>::iterator i = threads.begin(); i != threads.end(); i++){
        if ((*i).first == client_sockfd){
            threads.erase(i);
            break;
        }
    }
    return NULL;
}

int main(){
    int server_sockfd, client_sockfd;
	socklen_t server_len, client_len;
	struct sockaddr_in server_address;
	struct sockaddr_in client_address;
	
	server_sockfd = socket( AF_INET, SOCK_STREAM, 0 );
	
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_address.sin_port = htons(9734);
	server_len = sizeof(server_address);

	bind(server_sockfd, (struct sockaddr *)&server_address, server_len);
	listen(server_sockfd, 30);
    cout << "Server is up and running.\n";
	while (true) {
        client_sockfd = accept(server_sockfd, (struct sockaddr*)&client_address, &client_len);
        if (client_sockfd < 0)
        {
            cout << "Client connection error" << endl;
            continue;
        }
        cout << "Client number " << client_sockfd << " has entered.\n";
        pthread_t thread;
        pthread_create(&thread, NULL, &connection_handler, (int *) &client_sockfd);
        threads.push_back({client_sockfd, thread});
	}
	return 0;
}