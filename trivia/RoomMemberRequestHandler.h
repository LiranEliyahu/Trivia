#pragma once

#include "RoomRequestHandler.h"
#include "RequestHandlerFactory.h"

class RequestHandlerFactory;

class RoomMemberRequestHandler: public RoomRequestHandler
{
private:
	RequestHandlerFactory& m_handlerFactory;

public:
	RoomMemberRequestHandler(RequestHandlerFactory& handlerFactory, Room room, LoggedUser loggedUser);

	bool isRequestRelevant(RequestInfo requestInfo) const override;

private:
	RequestResult leaveRoom(RequestInfo requestInfo) override;
};

