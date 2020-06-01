#include <iostream>
#include <exception>
#include <thread>
#include <ctime>
#include <vector>

#include "Communicator.h"
#include "WSAInitializer.h"
#include "IRequestHandler.h"
#include "MenuRequestHandler.h"

#define MAX_BYTES_AMOUNT 1024

using std::exception;
using std::thread;
using std::string;
using std::cout;
using std::endl;
using std::ref;
using std::time;
using std::vector;

static const unsigned short PORT = 8826;
static const unsigned int INTERFACE = 0;


/*
	usage: constructor
	in: the handler factory
	out: no
*/
Communicator::Communicator(RequestHandlerFactory& handlerFactory) :
	m_handlerFactory(handlerFactory)
{
	this->_listeningSocket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (this->_listeningSocket == INVALID_SOCKET)
	{
		throw exception(__FUNCTION__ " - socket");
	}
}

/*
	usage: destructor
	in: no
	out: no
*/
Communicator::~Communicator()
{
	try
	{
		::closesocket(this->_listeningSocket);
	}
	catch (...) {}
}

/*
	this method sends the massage to the client (Hello) 
	and receives a message back,
	in case of an error in the process there's an exception 
	that can be throne

	input: the communicator and the client socket.
	output: none.
*/
void clientThread(Communicator* communicator, SOCKET clientSocket)
{
	while (true)
	{
		IRequestHandler* requestHandler = (*communicator)[clientSocket];
		vector<uint8_t> buffer(MAX_BYTES_AMOUNT + 1);

		if (recv(clientSocket, (char*)buffer.data(), MAX_BYTES_AMOUNT, 0) == INVALID_SOCKET)
		{
			communicator->removeClient(clientSocket);
			break;
		}

		RequestInfo requestInfo = { (MessageCode)buffer[0], time(nullptr), buffer };

		if (requestHandler->isRequestRelevant(requestInfo))
		{
			RequestResult requestResult = requestHandler->handleRequest(requestInfo);
			delete (*communicator)[clientSocket];
			(*communicator)[clientSocket] = requestResult.newHandler;

			if (send(clientSocket, (char*)requestResult.buffer.data(), requestResult.buffer.size(), 0) == INVALID_SOCKET)
			{
				communicator->removeClient(clientSocket);
				break;
			}
		}
	}
}

/*
	this method starts the binding and listening process
	and then she starts accepting clients by calling
	the handleNewClient method.

	input: none.
	output: none.
*/
void Communicator::startHandleRequests()
{
	bindAndListen();

	while (true)
	{
		Communicator::handleNewClient(Communicator::accept());
	}
}

/*
	usage: the method gets the handler factory
	in: no
	out: the handler factory
*/
RequestHandlerFactory& Communicator::getHandlerFactory()
{
	return this->m_handlerFactory;
}

void Communicator::removeClient(SOCKET clientSocket)
{
	this->m_clients.erase(clientSocket);
}

/*
	usage: the operator gets the request handler of socket
	in: the socket
	out: the request handler
*/
IRequestHandler*& Communicator::operator[](const SOCKET& socket)
{
	return this->m_clients[socket];
}

/*
	this method is meant for binding & listening procces.
	the method starts with binding and then listening and 
	if something went wrong then the an exception is being thrown.

	input: none.
	output: none.
*/
void Communicator::bindAndListen() const
{
	struct sockaddr_in socketAddress = { 0 };
	socketAddress.sin_port = htons(PORT);
	socketAddress.sin_family = AF_INET;
	socketAddress.sin_addr.s_addr = INTERFACE;

	if (::bind(this->_listeningSocket, (struct sockaddr*) & socketAddress, sizeof(socketAddress)) == SOCKET_ERROR)
	{
		throw exception(__FUNCTION__ " - bind");
	}

	if (::listen(this->_listeningSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		throw exception(__FUNCTION__ " - listen");
	}
}

/*
	this method gets the client socket and add the socket tot the 
	map and creates a new thread & detaches him.

	input: the client socket.
	output: none.
*/
void Communicator::handleNewClient(SOCKET clientSocket) 
{
	this->m_clients[clientSocket] = this->m_handlerFactory.createLoginRequestHandler();

	thread clientThread(clientThread, this, clientSocket);
	clientThread.detach();
}

/*
	this method accepts a new client and 
	creates a socket for the client and 
	returns the socket.

	input: none
	output: the client socket.
*/
SOCKET Communicator::accept() const
{
	SOCKET clientSocket = ::accept(this->_listeningSocket, NULL, NULL);

	if (clientSocket == INVALID_SOCKET)
	{
		throw exception(__FUNCTION__);
	}

	return clientSocket;
}
