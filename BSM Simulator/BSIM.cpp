#include<iostream>
#include<cstdio>
#include<winsock2.h>
#include <cstdlib>
#include <ctime>

using namespace std;

#define SERVER "10.10.71.159"
#define PORT 8888   //The port on which to listen for incoming data

struct BSM{
bool Dirty_flag;
int BSM_Id; 
int GPS_Fix; 
double Latitude; 
double Longitude; 
double Altitude;
double Lat_Error; 
double Long_Error; 
double Altitude_Error; 
double Speed; 
double Average_speed; 
double TimeStamp; 
};

int main(void)
{

	BSM bsm;
	
	struct sockaddr_in si_other;
    int s, slen=sizeof(si_other);
    WSADATA wsa;

    cout<<"\nInitialising Winsock..."<<endl;

    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
    {
        cout<<"Failed. Error Code : "<<WSAGetLastError();
        exit(EXIT_FAILURE);
    }

    cout<<"Initialised.\n";

    if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
    {
        cout<<"socket() failed with error code : "<<WSAGetLastError();
        exit(EXIT_FAILURE);
    }

    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORT);
    si_other.sin_addr.S_un.S_addr = inet_addr(SERVER);

	int counter = 0,r,size;
	
    while(counter < 20)
    {
	
		srand(time(NULL));
		r = (rand() % 10) + 1;
		//cout<<r<<endl;
		
        bsm.Dirty_flag = false;
		bsm.BSM_Id = r; 
		bsm.GPS_Fix = 1; 
		bsm.Latitude = 18; 
		bsm.Longitude = 73; 
		bsm.Altitude = 500;
		bsm.Lat_Error = 0; 
		bsm.Long_Error = 0; 
		bsm.Altitude_Error = 0; 
		bsm.Speed = r + 10; 
		bsm.Average_speed = 10; 
		bsm.TimeStamp = 123456; 
		
		size = sizeof(bsm);
        
		if (sendto(s, (char*)&bsm, size , 0 , (struct sockaddr *) &si_other, slen) == SOCKET_ERROR)
        {
            cout<<"sendto() failed with error code : "<<WSAGetLastError();
            exit(EXIT_FAILURE);
        }
		
		cout<<"Data sent: "<<counter<<endl;
		
		cout<<"\tBSM Id: "<<bsm.BSM_Id<<endl;
		cout<<"\tGPS Fix: "<<bsm.GPS_Fix<<endl; 
		cout<<"\tLatitude: "<<bsm.Latitude<<endl; 
		cout<<"\tLongitude: "<<bsm.Longitude<<endl; 
		cout<<"\tAltitude: "<<bsm.Altitude<<endl;
		cout<<"\tLatitude Error: "<<bsm.Lat_Error<<endl; 
		cout<<"\tLongitude Error: "<<bsm.Long_Error<<endl; 
		cout<<"\tAltitude Error: "<<bsm.Altitude_Error<<endl; 
		cout<<"\tSpeed: "<<bsm.Speed<<endl; 
		cout<<"\tAverage Speed: "<<bsm.Average_speed<<endl; 
		cout<<"\tDirty Flag: "<<bsm.Dirty_flag<<endl;
		cout<<"\tTimeStamp: "<<bsm.TimeStamp<<endl;
		
		cout<<endl<<endl;
		
    Sleep(2000);
	counter++;
    }

    closesocket(s);
    WSACleanup();

    return 0;
}