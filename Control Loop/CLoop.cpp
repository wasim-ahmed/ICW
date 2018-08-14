#include <iostream>
#include <list>
#include <map>
#include <vector>
#include <string>
#include <cstdio>
#include <chrono>
#include<winsock2.h>
#include <csignal>
using namespace std;

#define PORT 8888   //The port on which to listen for incoming data

void controlloop( int param);

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

std::vector<struct BSM> myvec;

int main()
{
	BSM bsm = {0};
    SOCKET s;
    struct sockaddr_in server, si_other;
    int slen , recv_len;
    int stlen = sizeof(bsm);
    WSADATA wsa;
	void (*prev_handler)(int);
	
	std::vector<struct BSM> tempvec;    
    slen = sizeof(si_other) ;

    cout<<"\nInitialising Winsock...";

    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
    {
        cout<<"Failed. Error Code : "<<WSAGetLastError();
        exit(EXIT_FAILURE);
    }

    cout<<"Initialised.\n";

    if((s = socket(AF_INET , SOCK_DGRAM , 0 )) == INVALID_SOCKET)
    {
        cout<<"Could not create socket : "<<WSAGetLastError();
    }

    cout<<"Socket created.\n";

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( PORT );

    if( bind(s ,(struct sockaddr *)&server , sizeof(server)) == SOCKET_ERROR)
    {
        cout<<"Bind failed with error code : "<<WSAGetLastError();
        exit(EXIT_FAILURE);
    }

    cout<<"Bind done"<<endl;
	
	
    int counter = 1;
	bool start = true;
	long bt,et;
	while(1)
    {
        //cout<<"Waiting for data...";
		cout<<"..."<<"\t";
		prev_handler = signal (SIGINT, controlloop);
		
		fflush(stdout);
        //clear the buffer by filling null, it might have previously received data
		                
        if ((recv_len = recvfrom(s, (char*)&bsm, stlen, 0, (struct sockaddr *) &si_other, &slen)) == SOCKET_ERROR)
        {
            cout<<"recvfrom() failed with error code : "<< WSAGetLastError();
            exit(EXIT_FAILURE);
        }
		
		auto now = std::chrono::system_clock::now();
		if(start == true)
		{
			bt = std::chrono::system_clock::to_time_t( now );
			//cout<<"bt:"<<bt<<endl;
			start = false;
		}
		
		//cout<<"Data received: "<<counter<<endl;
		//cout<<"number of bytes received: "<<recv_len<<endl;

       // cout<<"Received packet from"<<inet_ntoa(si_other.sin_addr)<<":"<<ntohs(si_other.sin_port)<<endl;
        
	/*	cout<<"\tBSM Id: "<<bsm.BSM_Id<<endl;
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
		//counter++;
		
		tempvec.push_back(bsm); 
		
		et = std::chrono::system_clock::to_time_t( now );
		//cout<<"et:"<<et<<endl;
		if((et - bt) >= 1)
		{
			//cout<<"tick"<<endl;
			raise(SIGINT);
			start = true;
			cout<<"total Packets received in this frame:"<<counter<<endl;
			counter = 0;
			myvec = tempvec;
			tempvec.clear();
		}
		counter++;
	}

	closesocket(s);
    WSACleanup();
	
}

void controlloop( int param)
{
		cout<<endl;
		cout<<__FUNCTION__<<endl;
	
		
		std::list<int> mylist;
		
		std::vector<struct BSM>::iterator itv;
	
		for(itv = myvec.begin(); itv != myvec.end(); itv++)
		{
			mylist.push_back(itv->BSM_Id);
		}
	
		mylist.sort();
		mylist.unique();
	
		std::list<int>::iterator it;

		cout<<"Unique BSM Id:"<<endl;
		
		for(it = mylist.begin(); it != mylist.end(); ++it)
		{
			cout<<*it<<endl;
		}

		//for(itv = myvec.begin(); itv != myvec.end(); itv++)
		{
		/*	cout<<itv->BSM_Id<<endl;
			cout<<itv->GPS_Fix<<endl; 
			cout<<itv->Latitude<<endl; 
			cout<<itv->Longitude<<endl; 
			cout<<itv->Altitude<<endl;
			cout<<itv->Lat_Error<<endl; 
			cout<<itv->Long_Error<<endl; 
			cout<<itv->Altitude_Error<<endl; 
			cout<<itv->Speed<<endl; 
			cout<<itv->Average_speed<<endl; 
			cout<<itv->TimeStamp<<endl; 
			cout<<itv->Dirty_flag<<endl;
		*/	
		}	
	
		std::map<int,vector<struct BSM>> mymap;
		std::map<int,vector<struct BSM>>::iterator itm; 
		
		for(it = mylist.begin(); it != mylist.end(); it++)
		{
			//cout<<*it<<endl;
			mymap[*it]; //Inserting Unique BSM Id's into map
		}
	
		for(itv = myvec.begin(); itv != myvec.end(); itv++)
		{
			itm = mymap.find(itv->BSM_Id);//finding the BSM ID in map
		
			if(itm != mymap.end())
			{
				//cout<<"Got It"<<itv->BSM_Id<<endl;
				mymap[itv->BSM_Id].push_back(*itv); //Pushing the BSM data (vector) on the same id
			}
			else
			{
				cout<<"BSM Id is not present in the map :: OBX BSM"<<endl;
			}
		}


	
	cout<<"****************************Current Frame Data*****************************************************"<<endl;
	
	for(itm = mymap.begin(); itm != mymap.end(); itm++) //To print the data
	{
		cout<<"BSM ID: "<<(*itm).first<<endl<<endl;
		
		vector<struct BSM> tempVec = (*itm).second;
		
		for(int i=0; i < tempVec.size(); i++)
		{
			cout<<"\tBSM Id: "<<tempVec[i].BSM_Id<<endl;
			cout<<"\tGPS Fix: "<<tempVec[i].GPS_Fix<<endl; 
			cout<<"\tLatitude: "<<tempVec[i].Latitude<<endl; 
			cout<<"\tLongitude: "<<tempVec[i].Longitude<<endl; 
			cout<<"\tAltitude: "<<tempVec[i].Altitude<<endl;
			cout<<"\tLatitude Error: "<<tempVec[i].Lat_Error<<endl; 
			cout<<"\tLongitude Error: "<<tempVec[i].Long_Error<<endl; 
			cout<<"\tAltitude Error: "<<tempVec[i].Altitude_Error<<endl; 
			cout<<"\tSpeed: "<<tempVec[i].Speed<<endl; 
			cout<<"\tAverage Speed: "<<tempVec[i].Average_speed<<endl; 
			cout<<"\tTimeStamp: "<<tempVec[i].TimeStamp<<endl; 
			cout<<"\tDirty Flag: "<<tempVec[i].Dirty_flag<<endl;
			
			cout<<endl;
		}
	}
	
	myvec.clear();
	mylist.clear();
	mymap.clear();
	//Sleep(15000);

	//return 0;
}

