//Compilation: CLoop.exe
//g++ -c -std=c++11 -pthread CLoop.cpp
//g++ -o CLoop.exe CLoop.o -l ws2_32
//Compiler required on windows : MinGW 4.8.1
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
#include <cmath>
using namespace std;

#define PORT 8888   //The port on which to listen for incoming data
#define PI 3.14159265

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
double calculateDistance(double,double,double,double);


std::vector<struct BSM> myvec; //the data of vehicle connected
std::vector<struct BSM> tempvec; //for receiving data from other vehicles
std::queue<struct predicted> myqueue;//Message Q to hold the predicted locations of vehicle in computation
std::condition_variable cv;//for synchronising threatAssessmentLoop

const int threatZone = 5;//in meters
const int confidenceLevel = 3;//in seconds
const int self_BSM_id = 21;
const int radius_of_earth = 6371;
const int dataPoint = 1;//This is in second. 1 second we will get 10 data points
const int periodicity = 100;//in ms
const int altitude_clearance  = 5;//in meters
bool lockX = false;//for synchronising threatAssessmentLoop


//Receives the data from other vehicles and fill the vector
int main()
{
	BSM bsm = {0};
    SOCKET s;
    struct sockaddr_in server, si_other;
    int slen , recv_len;
    int stlen = sizeof(bsm);
    WSADATA wsa;
	   
    slen = sizeof(si_other) ;
	
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
    {
        cout<<"Receiver Failed. Error Code : "<<WSAGetLastError();
        exit(EXIT_FAILURE);
    }

    if((s = socket(AF_INET , SOCK_DGRAM , 0 )) == INVALID_SOCKET)
    {
        cout<<"Receiver Could not create socket : "<<WSAGetLastError();
    }

    //cout<<"Receiver Socket created.\n";

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( PORT );

    if( bind(s ,(struct sockaddr *)&server , sizeof(server)) == SOCKET_ERROR)
    {
        cout<<"Receiver Bind failed with error code : "<<WSAGetLastError();
        exit(EXIT_FAILURE);
    }

    //cout<<"Receiver Bind done"<<endl;
	
	//Spawn the required threads
	thread time_mgmt(timeloop);
	thread connect(connectLoop);
	thread assess(threatAssessmentLoop,threatZone);
	while(1)
    {
		fflush(stdout);
        //clear the buffer by filling null, it might have previously received data
		                
        if ((recv_len = recvfrom(s, (char*)&bsm, stlen, 0, (struct sockaddr *) &si_other, &slen)) == SOCKET_ERROR)
        {
            cout<<"recvfrom() failed with error code : "<< WSAGetLastError();
            exit(EXIT_FAILURE);
        }
		
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
		mtx2.lock();//SEM2 Critical section Start
		tempvec.push_back(bsm); 
		mtx2.unlock();//SEM2 Critical section Ends	
	}
	closesocket(s);
    WSACleanup();
}

//This function send the ego vehicle data out at 10 Hz
//take the data from GPs/CAN Bus fill the data and send
void connectLoop()
{
	//cout<<__FUNCTION__<<endl;
	//The Logic of BSIM (send your BSM @ 10 Hz)
	
	BSM bsm;
	
	struct sockaddr_in si_other;
    int s, slen=sizeof(si_other);
    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
    {
        cout<<"Sender Failed. Error Code : "<<WSAGetLastError();
        exit(EXIT_FAILURE);
    }

    if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
    {
        cout<<"Sender socket() failed with error code : "<<WSAGetLastError();
        exit(EXIT_FAILURE);
    }
	int nOpt = 1;
	setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char*)&nOpt, sizeof(int));//for broadcast
    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORT);
    //si_other.sin_addr.S_un.S_addr = inet_addr(SERVER);
	si_other.sin_addr.s_addr = INADDR_BROADCAST;//broadcast

	
	int counter = 1,size;
	//maintain the periodicity of 10Hz and send BSM
	std::chrono::high_resolution_clock::time_point t1;
	std::chrono::high_resolution_clock::time_point t2;
	while(1)
	{
	
	//auto time = std::chrono::system_clock::now();//just to get the current time
	//std::time_t t_time = std::chrono::system_clock::to_time_t(time);
	//std::cout << "Started at------------- " << std::ctime(&t_time);
	//cout<<endl;
	
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
		((counter == 1) ? Sleep(1) : Sleep(wait_time));//wait to maintain 10 Hz periodicity
			

/*		std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
		auto duration = now.time_since_epoch();
		auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
		long long int temp;
		cout<<(millis)<<endl;
		cout<<"->->->"<<(millis - temp)<<endl;
		temp = millis;
		cout<<temp<<endl;
*/		
		//Fill in your data from GPS & CAN Bus
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
	}
    closesocket(s);
    WSACleanup();
}

//This function will gather the required amount of dataPoint 's & fill it in myvec vector .
//It strictly maintain the timing required for dataPoint like 1 sec for 10 data points, 2 sec for 20 dataPoint & so on.
void timeloop( )
{
	void (*prev_handler)(int);
	bool start = true;
	long bt,et;
	
	while(1)
	{
		prev_handler = signal (SIGINT, controlloop); //signal handler may be useful if it supports cloning
		auto now = std::chrono::system_clock::now();
		if(start == true)
		{
			bt = std::chrono::system_clock::to_time_t( now );
			//cout<<"bt:"<<bt<<endl;
			start = false;
		}
		
		Sleep(1);//Sleep for some time & don't eat the cpu
		
		et = std::chrono::system_clock::to_time_t( now );
		//cout<<"et:"<<et<<endl;
		if((et - bt) == dataPoint)
		{
			//cout<<"tick"<<endl;
			start = true;
		
			mtx1.lock();//SEM1 Critical section Start
			myvec = tempvec;
			mtx1.unlock();//SEM1 Critical section Ends	
		
			mtx2.lock();//SEM2 Critical section Start
			tempvec.clear();
			mtx2.unlock();//SEM2 Critical section Ends	
		
			raise(SIGINT);//Start the control as now we have latest data.
		}
	}
}

//Assuming Control Loop will take less then 1 sec
//This function will process the data gathered in last 1 sec
//and arrange the data w.r.t vehicles
void controlloop( int param)
{
		//cout<<endl;
		//cout<<__FUNCTION__<<endl;
		
	/*	auto time = std::chrono::system_clock::now(); //Just for checking the start time of loop
		std::time_t t_time = std::chrono::system_clock::to_time_t(time);
		std::cout << "Started at------------- " << std::ctime(&t_time);
		cout<<endl;
	*/
		std::list<int> mylist;//required to find the unique entry of vehicle as there will be multiple data from same vehicle
		
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
		//int connected_bsm = mylist.size();

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
	
		std::map<int,vector<struct BSM>> mymap;//required to arrange the data w.r.t vehicle
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
													//So that each vehicle will have it's history data points
			}
			else
			{
				cout<<"BSM Id is not present in the map :: OBX BSM"<<endl;
			}
		}

	vector<thread> threads;//vector to contain threads for individual vehicle
	
	for(itm = mymap.begin(); itm != mymap.end(); itm++) //Now the mymap contains the data arranged w.r.t vehicle we can spawn the threads
	{
		//cout<<"BSM ID: "<<(*itm).first<<endl<<endl;
		
		vector<struct BSM> localVec = (*itm).second;
		vector<struct BSM> passVec(localVec);
		
		threads.emplace_back(computationLoop,passVec); //require the parallel computation
				
		/*for(int i=0; i < localVec.size(); i++)
		{
			cout<<"\tBSM Id: "<<localVec[i].BSM_Id<<endl;
			cout<<"\tGPS Fix: "<<localVec[i].GPS_Fix<<endl; 
			cout<<"\tLatitude: "<<localVec[i].Latitude<<endl; 
			cout<<"\tLongitude: "<<localVec[i].Longitude<<endl; 
			cout<<"\tAltitude: "<<localVec[i].Altitude<<endl;
			cout<<"\tLatitude Error: "<<localVec[i].Lat_Error<<endl; 
			cout<<"\tLongitude Error: "<<localVec[i].Long_Error<<endl; 
			cout<<"\tAltitude Error: "<<localVec[i].Altitude_Error<<endl; 
			cout<<"\tSpeed: "<<localVec[i].Speed<<endl; 
			cout<<"\tAverage Speed: "<<localVec[i].Average_speed<<endl; 
			cout<<"\tTimeStamp: "<<localVec[i].TimeStamp<<endl; 
			cout<<"\tDirty Flag: "<<localVec[i].Dirty_flag<<endl;
			
			cout<<endl;
			
		}*/
	}
	
	mtx1.lock();//SEM1 Critical section Start
	myvec.clear();
	mtx1.unlock();//SEM1 Critical section Ends
	
	mylist.clear();//get ready for next iteration
	mymap.clear();//Clear the map as we have passed the data to respective threads & get ready for next iteration
	
	for(thread& t: threads)
	{
		t.join();//wait for them to finish execution 
	}
	
	threads.clear();//clear the vector & get ready for next iteration
	
	//time for threat assessment loop to start
	//Once all the computationLoop threads have finished the computation then start the threatAssessmentLoop
	{
	  std::unique_lock<std::mutex> lck(lock_mtx);
	  lockX = true;
	  cv.notify_all();
	}
}
//Each vehicle will have its computation loop in this it will correct the data (if required) & predict it.
void computationLoop(vector<struct BSM> vehicle)
{
		//cout<<__FUNCTION__<<endl;
		std::unique_lock<std::mutex> lck (atm);//just for maintaining the atomicity of thread, may be not required. No harm in either case !
		vector<struct BSM>::iterator it;
		//cout<<"Msg:"<<vehicle.size()<<endl;
		
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
		predicted p = {0}; //clear the structure
		p = predictLoop(vehicle,confidenceLevel);//predict
		myqueue.push(p);//put the data in Global Q of predicted points
}
//This function takes the history points(Lat&Long) and correct them in case of errors
void processLoop(vector<struct BSM>& vehicle)
{
	//cout<<__FUNCTION__<<endl;
	//Fix missing samples	NA
	
	//Fix spurious data & handle the dirty flag
				//take the Errors into consideration
		//double Lat_Error; 
		//double Long_Error; 
		//double Altitude_Error; 
		//GPS Fix
	
	//Altitude consideration NA
	
}
//Some one Please help in prediction ! :(
//Take the history points(Lat&Long) and predict for certain time (confidenceLevel)
struct predicted predictLoop(vector<struct BSM> vehicle,int confidenceLevel)
{
	//cout<<__FUNCTION__<<endl;
	cout<<"..............H		E	L	P............S		O	S......"<<endl;
	predicted p;
	
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
//This function will compute the distance between ego vehicle & other vehicles in computation. Declares if there is any threat.
void threatAssessmentLoop(int threatZone)
{
	//cout<<__FUNCTION__<<endl;
	double distance;
	predicted self,temp;
	std::vector<struct predicted> calcvec;
	//wait for all the threads to complete prediction so that assessment can be started, wait for signal !
	while(1)
	{
		std::unique_lock<std::mutex> lck(lock_mtx);//wait for the computation thread to finish
		while(!lockX) cv.wait(lck);					//wait for it's signal
	
		//cout<<"Got the Lock go ahead"<<endl;
	
		while(!myqueue.empty())
		{
			/*cout<<myqueue.front().BSM_Id<<endl;
			cout<<myqueue.front().Latitude<<endl; 
			cout<<myqueue.front().Longitude<<endl; 
			cout<<myqueue.front().Altitude<<endl;*/
			if(myqueue.front().BSM_Id == self_BSM_id)//own vehicles predicted points
			{
				self.BSM_Id = myqueue.front().BSM_Id;
				self.Latitude = myqueue.front().Latitude;
				self.Longitude = myqueue.front().Longitude;
				self.Altitude = myqueue.front().Altitude;
			}
			else{									//other vehicle predicted points
				temp.BSM_Id = myqueue.front().BSM_Id;
				temp.Latitude = myqueue.front().Latitude;
				temp.Longitude = myqueue.front().Longitude;
				temp.Altitude = myqueue.front().Altitude;
				calcvec.push_back(temp); //push it local vector of other vehicles predicted points
			}
			
			myqueue.pop();//empty the Q for next cycle
		}
		
		//myqueue.empty() == true ? cout<<"Queue is empty"<<endl : cout<<"Queue is not empty"<<endl;
		
			/*cout<<"Self predicted data:"<<endl;
			cout<<self.BSM_Id<<endl;
			cout<<self.Latitude<<endl;
			cout<<self.Longitude<<endl;
			cout<<self.Altitude<<endl;
			
			cout<<endl<<endl;
			*/
			//cout<<"Rest prediction data:"<<endl;
		vector<struct predicted>::iterator it;
		for(it = calcvec.begin();it != calcvec.end();it++)
		{
			/*	cout<<it->BSM_Id<<endl;
				cout<<it->Latitude<<endl;
				cout<<it->Longitude<<endl;
				cout<<it->Altitude<<endl;
				cout<<endl;
			*/
			//function call to calculate threat
			double lat1 = self.Latitude * (PI/180);
			double long1 = self.Longitude * (PI/180);
			double lat2 = it->Latitude * (PI/180);
			double long2 = it->Longitude * (PI/180);

			distance = calculateDistance(lat1,long1,lat2,long2);//calculate the distance between ego vehicle & other vehicle  
			//consider altitude
			int altitude_difference = abs(self.Altitude - it->Altitude);
			if(distance <= threatZone && altitude_difference <= altitude_clearance) //If distance is less than safe distance & on the same altitude
			{
				cout<<"************brace yourself for impact with "<<it->BSM_Id<<" ************"<<endl;
			}
		}
			
		//cout<<"vector size"<<calcvec.size()<<endl;
			
		calcvec.clear();	
		lockX = false;//make the lock unavailable
	}
}
//This function will calculate the distance between 2 points in meters
double calculateDistance(double lat1,double long1,double lat2,double long2)
{
	double diflat = lat2 - lat1;
	double diflong = long2 - long1;
	double a,c,d;
	double R = radius_of_earth;

	a = pow((sin(diflat/2)),2) + cos(lat1)*cos(lat2)* pow((sin(diflong/2)),2);
	c = 2* atan2( sqrt(a), sqrt(1-a));
	d = R*c;
	return (d * 1000);//in meters
}