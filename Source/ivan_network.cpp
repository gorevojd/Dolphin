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
	FUNCTION: ParseIPv4;
	
	PURPOSE: Parse IPv4 address to unsigned int(32 bit) value;

	PARAMS: 
		IP - address to parse; 
		ResultParsedIP - destination value where parsed result is written;

	RETURN CODES: Function succeed when it returns 0;
*/
INTERNAL_FUNCTION int ParseIPv4(char* IP, u32* ResultParsedIP) {
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