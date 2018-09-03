//A Static BSM Simulator
//Compiling BSIM.exe
//g++ -c -std=c++11 BSIM.cpp
//g++ -o BSIM.exe BSIM.o -l ws2_32
//Compiler required on windows : MinGW

#include<iostream>
#include<cstdio>
#include<winsock2.h>
#include <cstdlib>
#include <ctime>
#include <set>
#include <conio.h>
#include <chrono>
#include <ratio>

using namespace std;

//#define SERVER "10.10.71.159"
#define PORT 8888   //The port on which to listen for incoming data
#define history 10

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
long TimeStamp; 
};

const int periodicity = 100;//in ms

//Simulate the sending BSM of Traffic
//First randomly generate the number of cars participating
//Then randomly generate the BSM Id's of these cars
//Send the data @ 10 Hz
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
	int nOpt = 1;
	setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char*)&nOpt, sizeof(int));//for broadcast
    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORT);
    //si_other.sin_addr.S_un.S_addr = inet_addr(SERVER);
	si_other.sin_addr.s_addr = INADDR_BROADCAST;//broadcast

	int No_of_BSM;
	srand(time(NULL));
	
	No_of_BSM = (rand() % 10) + rand() % 10; //how many Cars to generate in this frame
	
	cout<<"number of BSM's generated in this cycle:"<<No_of_BSM<<endl;

	int BSM_Id_List[No_of_BSM];
	for(int i=0; i <No_of_BSM; i++)
	{
		int BSM_Id;
		srand(No_of_BSM + i);
		BSM_Id = (rand() % 10) + 1;
		BSM_Id_List[i] = BSM_Id; //Car Id's generated in this frame
	}
	cout<<"BSM Id's generated in this cycle:"<<endl;
	for(int i=0; i <No_of_BSM; i++)
	{
		cout<<BSM_Id_List[i]<<endl;
	}

	std::set<int> BSM_Set(BSM_Id_List,(BSM_Id_List + No_of_BSM));
	std::set<int>::iterator it;
	
	cout<<"Unique BSM Id's generated:"<<endl;
	for(it = BSM_Set.begin(); it != BSM_Set.end(); it++)
	{
		cout<<*it<<endl;
	}
	
	int counter = 1,size,counterX = 1;
	
	std::chrono::high_resolution_clock::time_point t1;
	std::chrono::high_resolution_clock::time_point t2;
	while(1)
	{
	
	auto time = std::chrono::system_clock::now();
	std::time_t t_time = std::chrono::system_clock::to_time_t(time);
	//std::cout << "Started at------------- " << std::ctime(&t_time);
	//cout<<endl;
	
		while(counter <= history)
		{
		
			t2 = std::chrono::high_resolution_clock::now();
							
			std::chrono::duration<double, std::milli> time_span = t2 - t1;
				

		//	cout << "It took me------------------------------- " << time_span.count() << " milliseconds.";
		//	cout << endl;
			
			double wait_time = 0;	
			double abs_wait_time = periodicity - time_span.count();
			if(abs_wait_time < 0)
			{
				wait_time = 0;
			}
			else
			{
				wait_time = abs_wait_time;
			}
			
		//	cout << "wait time------------------------------- "<< wait_time<<endl;
			((counterX == 1) ? Sleep(1) : Sleep(wait_time));//maintain the 10 Hz frequency
			
			counterX++;
				

	/*		std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
			auto duration = now.time_since_epoch();
			auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
			long long int temp;
			cout<<(millis)<<endl;
			cout<<"->->->"<<(millis - temp)<<endl;
			temp = millis;
			cout<<temp<<endl;
	*/		
		//	cout<<"Data Set: "<<counter<<endl;
			int inner_counter = 1;
			for(it = BSM_Set.begin(); it != BSM_Set.end(); it++)
			{
				//Static data !
				bsm.Dirty_flag = false;
				bsm.BSM_Id = *it; 
				bsm.GPS_Fix = counter; 
				if(bsm.BSM_Id == 2)
				{
					bsm.GPS_Fix = 0;//just to check Process Loop. So the BSM id 2 will be discarded
				}
				bsm.Latitude = 18; 
				bsm.Longitude = 73; 
				bsm.Altitude = 500;
				bsm.Lat_Error = 0; 
				bsm.Long_Error = 0; 
				bsm.Altitude_Error = 0; 
				bsm.Speed = *it + 10; 
				bsm.Average_speed = 10; 
				bsm.TimeStamp = 12345;//std::chrono::system_clock::to_time_t( now ); 
				
				size = sizeof(bsm);

				if (sendto(s, (char*)&bsm, size , 0 , (struct sockaddr *) &si_other, slen) == SOCKET_ERROR)
				{
					cout<<"sendto() failed with error code : "<<WSAGetLastError();
					exit(EXIT_FAILURE);
				}
				if(inner_counter == 1)
				{
					t1 = std::chrono::high_resolution_clock::now();
				}
				inner_counter++;
				
		/*		cout<<"\tBSM Id: "<<bsm.BSM_Id<<endl;
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
		*/
			}
			counter++;
		}
	counter = 1;
	}
    closesocket(s);
    WSACleanup();

    return 0;
}