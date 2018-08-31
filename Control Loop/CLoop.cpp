#include <iostream>
#include <list>
#include <map>
#include <vector>
#include <string>
#include <cstdio>
#include <chrono>
#include<winsock2.h>
#include <csignal>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
using namespace std;

#define PORT 8888   //The port on which to listen for incoming data

mutex mtx1;
mutex mtx2;
mutex atm;
mutex lock_mtx;

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

struct predicted
{
	int BSM_Id; 
	double Latitude; 
	double Longitude;
	double Altitude;	
};

void connectLoop();
void controlloop( int param);
void timeloop( );
void computationLoop(vector<struct BSM>);
void processLoop(vector<struct BSM>&);
struct predicted predictLoop(vector<struct BSM>,int confidenceLevel);
void threatAssessmentLoop(int threatZone);


std::vector<struct BSM> myvec;
std::vector<struct BSM> tempvec; 
std::queue<struct predicted> myqueue;
std::condition_variable cv;

const int threatZone = 1;
const int confidenceLevel = 3;
const int self_BSM_id = 21;
bool lockX = false;


int main()
{
	int dataPoint = 1;//This is in second assuming in 1 second we will get 10 data points
	
	
	BSM bsm = {0};
    SOCKET s;
    struct sockaddr_in server, si_other;
    int slen , recv_len;
    int stlen = sizeof(bsm);
    WSADATA wsa;
	   
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
	
	//int counter = 1;
	thread time_mgmt(timeloop);
	thread connect(connectLoop);
	thread assess(threatAssessmentLoop,threatZone);
	while(1)
    {
        //cout<<"Waiting for data...";
		//cout<<"..."<<"\t";
		
		fflush(stdout);
        //clear the buffer by filling null, it might have previously received data
		                
        if ((recv_len = recvfrom(s, (char*)&bsm, stlen, 0, (struct sockaddr *) &si_other, &slen)) == SOCKET_ERROR)
        {
            cout<<"recvfrom() failed with error code : "<< WSAGetLastError();
            exit(EXIT_FAILURE);
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
		//SEM2 Critical section Start
		mtx2.lock();
		tempvec.push_back(bsm); 
		mtx2.unlock();
		//SEM2 Critical section Ends	
		//counter++;
	}


	//time_mgmt.join(); //not required , may be
	closesocket(s);
    WSACleanup();
}
void connectLoop()
{
	cout<<__FUNCTION__<<endl;
	//The Logic of BSIM (send your BSM @ 10 Hz)
	
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

	//while(1)
	
	//getch();
	
	int counter = 1,size;
	
	std::chrono::high_resolution_clock::time_point t1;
	std::chrono::high_resolution_clock::time_point t2;
	while(1)
	{
	
	//auto time = std::chrono::system_clock::now();
	//std::time_t t_time = std::chrono::system_clock::to_time_t(time);
	//std::cout << "Started at------------- " << std::ctime(&t_time);
	//cout<<endl;
	
	
		t2 = std::chrono::high_resolution_clock::now();
						
		std::chrono::duration<double, std::milli> time_span = t2 - t1;
			

	//	cout << "It took me------------------------------- " << time_span.count() << " milliseconds.";
	//	cout << endl;
		
		double wait_time = 0;	
		double abs_wait_time = 100 - time_span.count();
		if(abs_wait_time < 0)
		{
			wait_time = 0;
		}
		else
		{
			wait_time = abs_wait_time;
		}
		
	//	cout << "wait time------------------------------- "<< wait_time<<endl;
		((counter == 1) ? Sleep(1) : Sleep(wait_time));
			

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
				//Fill in your data
			bsm.Dirty_flag = false;
			bsm.BSM_Id = 21; 
			bsm.GPS_Fix = 1; 
			bsm.Latitude = 18; 
			bsm.Longitude = 73; 
			bsm.Altitude = 500;
			bsm.Lat_Error = 0; 
			bsm.Long_Error = 0; 
			bsm.Altitude_Error = 0; 
			bsm.Speed = 10; 
			bsm.Average_speed = 10; 
			bsm.TimeStamp = 12345;//std::chrono::system_clock::to_time_t( now ); 
			
			size = sizeof(bsm);

			if (sendto(s, (char*)&bsm, size , 0 , (struct sockaddr *) &si_other, slen) == SOCKET_ERROR)
			{
				cout<<"sendto() failed with error code : "<<WSAGetLastError();
				exit(EXIT_FAILURE);
			}
			
			t1 = std::chrono::high_resolution_clock::now();
			
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
		
		counter++;
 	
	//cout<<"------------------------------------------------------------------------------------------------------------------"<<endl;
	}
    closesocket(s);
    WSACleanup();

   
}
void timeloop( )
{
	void (*prev_handler)(int);
	bool start = true;
	long bt,et;
	int dataPoint = 1;//make it 2 for 20 data points & so forth
	while(1)
	{
	//cout<<__FUNCTION__<<endl;
		prev_handler = signal (SIGINT, controlloop); //signal handler may be useful if it supports cloning
		auto now = std::chrono::system_clock::now();
		if(start == true)
		{
			bt = std::chrono::system_clock::to_time_t( now );
			//cout<<"bt:"<<bt<<endl;
			start = false;
		}
		
		Sleep(1);
		et = std::chrono::system_clock::to_time_t( now );
		//cout<<"et:"<<et<<endl;
		if((et - bt) == dataPoint)
		{
			//cout<<"tick"<<endl;
			start = true;
			//cout<<"total Packets received in this frame:"<<counter<<endl;
			//counter = 0;
		//SEM1 Critical section Start
			mtx1.lock();
			myvec = tempvec;
			mtx1.unlock();
		//SEM1 Critical section Ends	
		//SEM2 Critical section Start
			mtx2.lock();
			tempvec.clear();
			mtx2.unlock();
		//SEM2 Critical section Ends	
			raise(SIGINT);
			
		}
	
	}

}
//Assuming Control Loop will take less then 1 sec
void controlloop( int param)
{
		//cout<<endl;
		//cout<<__FUNCTION__<<endl;
		
	/*	auto time = std::chrono::system_clock::now();
		std::time_t t_time = std::chrono::system_clock::to_time_t(time);
		std::cout << "Started at------------- " << std::ctime(&t_time);
		cout<<endl;
	*/
		std::list<int> mylist;
		
		std::vector<struct BSM>::iterator itv;
	
		for(itv = myvec.begin(); itv != myvec.end(); itv++)
		{
			mylist.push_back(itv->BSM_Id);
		}
	
		mylist.sort();
		mylist.unique();
	
		std::list<int>::iterator it;

		//cout<<"Unique BSM Id:"<<endl;
		
		for(it = mylist.begin(); it != mylist.end(); ++it)
		{
			//cout<<*it<<endl;
		}
		int connected_bsm = mylist.size();

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

	//cout<<"****************************Current Frame Data*****************************************************"<<endl;
	vector<thread> threads;
	for(itm = mymap.begin(); itm != mymap.end(); itm++) //To print the data
	{
		//cout<<"BSM ID: "<<(*itm).first<<endl<<endl;
		
		vector<struct BSM> tempVec = (*itm).second;
		vector<struct BSM> passVec(tempVec);
		
		//thread computationloop(P2Loop,passVec);//spawning a thread & waiting for it's execution
		//computationloop.join();	
		//require the parallel computation
			
		threads.emplace_back(computationLoop,passVec);
 
		//P2Loop(passVec);//in case if you want a function
		
		/*for(int i=0; i < tempVec.size(); i++)
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
			
		}*/
		
	}
	
	//SEM1 Critical section Start
	mtx1.lock();
	myvec.clear();
	//SEM1 Critical section Ends
	mtx1.unlock();
	mylist.clear();
	mymap.clear();
	//Sleep(15000);

	for(thread& t: threads)
	{
		t.join();//wait for them to finish execution 
	}
	
	threads.clear();
	//time for threat assessment loop to start
		{
		  std::unique_lock<std::mutex> lck(lock_mtx);
		  lockX = true;
		  cv.notify_all();
		}
	//return 0;
}

void computationLoop(vector<struct BSM> vehicle)
{
		//cout<<__FUNCTION__<<endl;
		std::unique_lock<std::mutex> lck (atm);//just for maintaining the atomicity of thread
		vector<struct BSM>::iterator it;
		cout<<"Msg:"<<vehicle.size()<<endl;
		
	/*	for(it = vehicle.begin(); it != vehicle.end(); it++)
		{
			cout<<"BSM Id: "<<it->BSM_Id<<endl;
			cout<<"\tGPS Fix: "<<it->GPS_Fix<<endl; 
			cout<<"\tLatitude: "<<it->Latitude<<endl; 
			cout<<"\tLongitude: "<<it->Longitude<<endl; 
			cout<<"\tAltitude: "<<it->Altitude<<endl;
			cout<<"\tLatitude Error: "<<it->Lat_Error<<endl; 
			cout<<"\tLongitude Error: "<<it->Long_Error<<endl; 
			cout<<"\tAltitude Error: "<<it->Altitude_Error<<endl; 
			cout<<"\tSpeed: "<<it->Speed<<endl; 
			cout<<"\tAverage Speed: "<<it->Average_speed<<endl; 
			cout<<"\tTimeStamp: "<<it->TimeStamp<<endl; 
			cout<<"\tDirty Flag: "<<it->Dirty_flag<<endl;
			
			cout<<endl;
		}
	*/	
		processLoop(vehicle);//sanity test 
		predicted p = {0};
		p = predictLoop(vehicle,confidenceLevel);//predict
		myqueue.push(p);
}

void processLoop(vector<struct BSM>& vehicle)
{
	//cout<<__FUNCTION__<<endl;
	//Fix missing samples	NA
	
	//Fix spurious data & handle the dirty flag
				//take the Errors into consideration
		//double Lat_Error; 
		//double Long_Error; 
		//double Altitude_Error; 
	
	//Altitude consideration NA
	
}

struct predicted predictLoop(vector<struct BSM> vehicle,int confidenceLevel)
{
	//cout<<__FUNCTION__<<endl;
	cout<<"..............H		E	L	P............S		O	S......"<<endl;
	predicted p;
	//p.Latitude = 1; 
	//p.Longitude = 1; 
	//p.Altitude;
	//p.BSM_Id;
	
	vector<struct BSM>::iterator it;
		for(it = vehicle.begin(); it != vehicle.end(); it++)
		{
		/*
			cout<<"BSM Id: "<<it->BSM_Id<<endl;
			cout<<"\tGPS Fix: "<<it->GPS_Fix<<endl; 
			cout<<"\tLatitude: "<<it->Latitude<<endl; 
			cout<<"\tLongitude: "<<it->Longitude<<endl; 
			cout<<"\tAltitude: "<<it->Altitude<<endl;
			cout<<"\tLatitude Error: "<<it->Lat_Error<<endl; 
			cout<<"\tLongitude Error: "<<it->Long_Error<<endl; 
			cout<<"\tAltitude Error: "<<it->Altitude_Error<<endl; 
			cout<<"\tSpeed: "<<it->Speed<<endl; 
			cout<<"\tAverage Speed: "<<it->Average_speed<<endl; 
			cout<<"\tTimeStamp: "<<it->TimeStamp<<endl; 
			cout<<"\tDirty Flag: "<<it->Dirty_flag<<endl;
		*/	
			p.Latitude = it->Latitude + 0.001; 
			p.Longitude = it->Longitude + 0.001; 
			p.Altitude = it->Altitude;
			p.BSM_Id = it->BSM_Id;
			
			//cout<<endl;
		}
	
	return p;
}

void threatAssessmentLoop(int threatZone)
{
	//cout<<__FUNCTION__<<endl;
	predicted self,temp;
	std::vector<struct predicted> calcvec;
	//wait foe all the threads to complete prediction, wait for signal
	while(1)
	{
	std::unique_lock<std::mutex> lck(lock_mtx);
	while(!lockX) cv.wait(lck);
	
	//cout<<"Got the Lock go ahead"<<endl;
	
		while(!myqueue.empty())
		{
			/*cout<<myqueue.front().BSM_Id<<endl;
			cout<<myqueue.front().Latitude<<endl; 
			cout<<myqueue.front().Longitude<<endl; 
			cout<<myqueue.front().Altitude<<endl;*/
			if(myqueue.front().BSM_Id == self_BSM_id)
			{
				self.BSM_Id = myqueue.front().BSM_Id;
				self.Latitude = myqueue.front().Latitude;
				self.Longitude = myqueue.front().Longitude;
				self.Altitude = myqueue.front().Altitude;
			}
			else{
				temp.BSM_Id = myqueue.front().BSM_Id;
				temp.Latitude = myqueue.front().Latitude;
				temp.Longitude = myqueue.front().Longitude;
				temp.Altitude = myqueue.front().Altitude;
				calcvec.push_back(temp);
			}
			
			
			
			myqueue.pop();
		}
		
		//myqueue.empty() == true ? cout<<"COOL"<<endl : cout<<"Nama Cool"<<endl;
		
			cout<<"Self predicted data:"<<endl;
			cout<<self.BSM_Id<<endl;
			cout<<self.Latitude<<endl;
			cout<<self.Longitude<<endl;
			cout<<self.Altitude<<endl;
			
			cout<<endl<<endl;

			cout<<"Rest prediction data:"<<endl;
			vector<struct predicted>::iterator it;
			for(it = calcvec.begin();it != calcvec.end();it++)
			{
				cout<<it->BSM_Id<<endl;
				cout<<it->Latitude<<endl;
				cout<<it->Longitude<<endl;
				cout<<it->Altitude<<endl;
				cout<<endl;
			}
			
			cout<<"vector size"<<calcvec.size()<<endl;
		
	calcvec.clear();	
	lockX = false;//make the lock unavailable
	}
}
