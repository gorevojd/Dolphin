#include "ivan_network.h"

#define IVAN_NETWORK_IP_PARSE_SUCCESS 0
#define IVAN_NETWORK_IP_PARSE_INVALID_DOT_COUNT 1
#define IVAN_NETWORK_IP_PARSE_INVALID_OCTETS 2

INTERNAL_FUNCTION int ParseIPOctetInternal(char* Begin, char* OnePastEnd, u32* ResultOctet) {
	int Result = IVAN_NETWORK_IP_PARSE_SUCCESS;

	*ResultOctet = 0;

	int TenStack = 1;
	if (OnePastEnd - Begin) {
		char* ScanAt = OnePastEnd - 1;

		while (ScanAt >= Begin) {
			*ResultOctet += TenStack * (*ScanAt - '0');
			TenStack *= 10;

			ScanAt--;
		}
	}
	else {
		Result = IVAN_NETWORK_IP_PARSE_INVALID_OCTETS;
	}

	return(Result);
}


/*
	FUNCTION: IPv4FromASCII;
	
	PURPOSE: Parse IPv4 address to unsigned int(32 bit) value;

	PARAMS: 
		IP - address to parse; 
		ResultParsedIP - destination value where parsed result is written;

	RETURN CODES: Function succeed when it returns 0;
*/
INTERNAL_FUNCTION int ParseIPv4Internal(char* IP, u32* ResultParsedIP) {
	int Result = 0;

	*ResultParsedIP = 0;

	char* At = IP;
	char* OctetBegin = At;

	int DotCount = 0;
	while (*At) {

		if (*At == '.') {
			u32 CurrentOctet;
			int ParseOctet = ParseIPOctetInternal(OctetBegin, At, &CurrentOctet);
			if (ParseOctet == IVAN_NETWORK_IP_PARSE_SUCCESS) {
				*ResultParsedIP |= (CurrentOctet << (DotCount << 3));
			}
			else {
				Result = ParseOctet;
				break;
			}

			DotCount++;
			OctetBegin = At + 1;
		}

		At++;
	}

	if (*At == 0) {
		//NOTE(Dima): Last octet
		if (DotCount != 3) {
			Result = IVAN_NETWORK_IP_PARSE_INVALID_DOT_COUNT;
		}
		else {
			u32 LastOctet;
			int ParseOctet = ParseIPOctetInternal(OctetBegin, At, &LastOctet);
			if (ParseOctet == IVAN_NETWORK_IP_PARSE_SUCCESS) {
				*ResultParsedIP |= (LastOctet << (DotCount << 3));
			}
			else {
				Result = ParseOctet;
			}
		}
	}

	return(Result);
}

INTERNAL_FUNCTION u32 IPv4FromASCII(char* IP){
	u32 Result;

	ParseIPv4Internal(IP, &Result);

	return(Result);
}


#define IVAN_NETWORK_IP_VALID 0
#define IVAN_NETWORK_IP_INVALID 1
INTERNAL_FUNCTION b32 CheckIP(char* IP, u32* Out){
	b32 Result = IVAN_NETWORK_IP_VALID;

	if(ParseIPv4Internal(IP, Out)){
		Result = IVAN_NETWORK_IP_INVALID;
	}

	return(Result);
}

INTERNAL_FUNCTION void OctetToASCII(u32 Octet, char* Dest) {

	char* At = Dest;
	char* Begin = Dest;
	char* End = 0;

	do {
		u32 CurrentCharValue = Octet % 10;
		Octet /= 10;
		char ToWrite = '0' + CurrentCharValue;
		*At++ = ToWrite;
	} while (Octet);

	End = At;

	while (End > Begin) {
		--End;
		char TempChar = *Begin;
		*Begin = *End;
		*End = TempChar;
		++Begin;
	}

	*At = 0;
}

INTERNAL_FUNCTION void WriteOctetToDest(char* Octet, char* Dest) {
	char* At = Octet;

	while (*At++) {
		*Dest++;
	}
}

INTERNAL_FUNCTION void IPv4ToASCII(u32 IP, char* Dest) {
	char Temp[32];

	char* DestAt = Dest;

	for (int OctetIndex = 0;
		OctetIndex < 4;
		OctetIndex++)
	{
		u32 Octet = (IP >> (OctetIndex << 3)) & 0xFF;

		OctetToASCII(Octet, Temp);

		char* At = Temp;
		while (*At) {
			*DestAt++ = *At++;
		}

		if (OctetIndex == 3) {
			*DestAt = 0;
		}
		else {
			*DestAt++ = '.';
		}
	}
}