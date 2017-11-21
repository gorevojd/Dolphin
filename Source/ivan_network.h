#ifndef IVAN_NETWORK_H
#define IVAN_NETWORK_H

enum client_connection_state{
	ClientConnectionState_Disconnected,
	ClientConnectionState_Reserved,
	ClientConnectionState_Connected,
};

struct network_client{
	volatile uint32 ConnectionState;

	uint32 IP;
	uint16 Port;
};

#define IVAN_NETWORK_MAX_CLIENTS 4
struct network_server{
	network_client Clients[IVAN_NETWORK_MAX_CLIENTS];
};

inline bool32 IsClientConnected(network_server* Server, int ClientIndex){
	bool32 Result = false;

	if(Server->Clients[ClientIndex].ConnectionState == ClientConnectionState_Connected){
		Result = true;
	}

	return(Result);
}

inline network_client* GetEmptyClientSlot(network_server* Server){
	network_client* Result = 0;

	for(int ClientIndex = 0; ClientIndex < IVAN_NETWORK_MAX_CLIENTS; ClientIndex){
		network_client* Client = &Server->Clients[ClientIndex];	
		if(Client->State == ClientConnectionState_Disconnected){
			Result = Client;
			break;
		}
	}

	return(Result);
}

#endif