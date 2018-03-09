//Kenzie Wong 
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>     /* atof */
#include <thread> //thread library
#include <chrono> //Timing 
#include <mutex> //mutex life & unique_lock
#include <condition_variable> //C.V.s

using namespace std;

//Node of queue
struct train_data{
	int train_num;
	string direction;
	int ldTime;
	int xTime;
	int ready = 0;
	
	train_data *next;
};

//double linked list
class linked{
	public:
		void push(int train_num, string direction, int ldTime, int xTime);
		int peek_train();
		int peek();
		void prep();
		int is_prep();
		void pop();
		//void print(); //used for testing.
	private:
		train_data *head;	
	
};

void linked::push(int train_num, string direction, int ldTime, int xTime){
	train_data *node = new train_data; //create new node and insert data.
	node->train_num = train_num;
	node->direction = direction;
	node->ldTime = ldTime;
	node->xTime = xTime;
	if(head == NULL){ //linked is empty.
	head = node;
	}else if(/*(head->ldTime > ldTime)||*/(head->ldTime == ldTime && head->train_num > train_num)){ //goes before current head. 
		//might be wrong timing. remove (head->ldTime > ldTime) to remove this check.
		node->next = head;
		head = node;
	}else{ // goes after head somewhere
		struct train_data *tmp;
		tmp = head;
		bool keep = true;
		while(keep){
			if(tmp->next == NULL){ // at end of list. append.
				tmp->next = node;
				keep = false;
			}else if((tmp->next->ldTime > ldTime)||(tmp->next->ldTime == ldTime && tmp->next->train_num > train_num)){
				node->next = tmp->next;
				tmp->next = node;
				keep = false;
			}else{
				tmp = tmp->next;
			}
		}
	}
}
int linked::peek_train(){ //checks which train is at head.
	return head->train_num;
}
int linked::peek(){ //checks is empty.
	if(head == NULL){
		return 0;
	}else{
		return 1;
	}
}
void linked::prep(){ //changes ready int.
	head->ready = 1;
}
int linked::is_prep(){ //checks ready int.
	return head->ready;
}
void linked::pop(){ //remove from list.
	if(head == NULL){
		cout << "system error. trying to pop empty station.";
	}else{
		head = head->next;
	}
}

//used for testing.
/*
void linked::print(){
	struct train_data *tmp;
	cout << "train order: \n";
	tmp = head;
	int i = 0;
	
	while(tmp != NULL){
		i++;
		cout << " train# " << tmp->train_num << "\n";
		tmp = tmp->next; 		
	}
}
*/
//global variables
int number_of_trains = 0;

//Lists
linked station_W;
linked station_E;
linked station_w;
linked station_e;

mutex qW; //W queue
mutex qE; //E queue
mutex qwst; //w queue
mutex qest; //e queue
mutex track; //track
//condition variables
condition_variable cvW, cvE, cv_wst, cv_est, cvT;

//global timer
chrono::system_clock::time_point timer;


void print_time(int train, int status, string direction){
	auto curr = chrono::system_clock::now(); //get current time
	auto diff = curr - timer; //calculate difference
	auto milliseconds = (diff/100000000)%10;
	auto seconds = (diff/1000000000)%60;
	auto minutes =(diff/60000000000)%60;
	auto hours =(diff/3600000000000);

	if(status == 1){
		printf("%ld:%02ld:%02ld.%ld Train %d is ready to go %s \n", (hours).count(), (minutes).count(), (seconds).count(), (milliseconds).count(), train, direction.c_str());
	
	}else if(status == 2){
		printf("%ld:%02ld:%02ld.%ld Train %d is ON the main track going %s \n", (hours).count(), (minutes).count(), (seconds).count(), (milliseconds).count(), train, direction.c_str());
	}else{
		printf("%ld:%02ld:%02ld.%ld Train %d is OFF the main track after going %s \n", (hours).count(), (minutes).count(), (seconds).count(), (milliseconds).count(), train, direction.c_str());
	}
}


//train threads
void chu_chu_train(train_data *temp){

	unique_lock<mutex> lck(track);
	
	cvT.wait(lck);
	lck.unlock();
	int ld = temp->ldTime;
	this_thread::sleep_for(chrono::milliseconds(ld*100));
	if(temp->direction == "W"){
		unique_lock<mutex> Wlck(qW); //Locks
		station_W.push(temp->train_num, temp->direction, temp->ldTime, temp->xTime);
		print_time(temp->train_num, 1, "West");
	
		while(station_W.is_prep() != 1 || station_W.peek_train() != temp->train_num){ //While first train is not rdy and this train is not next loop while
			cvW.wait(Wlck);
		}
		station_W.pop(); //remove thread
		qW.unlock(); //unlocks queue
		lck.lock(); //locks for crossing
		int cross = temp->xTime;
		print_time(temp->train_num, 2, "West");
		this_thread::sleep_for(chrono::milliseconds(cross*100)); //cross time
		print_time(temp->train_num, 3, "West");
		lck.unlock(); //release track
		cvT.notify_one(); //notify dispatcher
	}else if(temp->direction == "E"){
		unique_lock<mutex> Elck(qE); //Locks
		
		station_E.push(temp->train_num, temp->direction, temp->ldTime, temp->xTime);
		print_time(temp->train_num, 1, "East");
			
		while(station_E.is_prep() != 1 || station_E.peek_train() != temp->train_num){ //While first train is not rdy and this train is not next loop while
			cvE.wait(Elck);
		}
		station_E.pop(); //removes thread
		qE.unlock(); //unlocks queue	
		lck.lock(); //locks for crossing
		int cross = temp->xTime;
		print_time(temp->train_num, 2, "East");
		this_thread::sleep_for(chrono::milliseconds(cross*100)); //cross time
		print_time(temp->train_num, 3, "East");
		lck.unlock(); //release track
		cvT.notify_one(); //notify dispatcher
		
	}else if(temp->direction == "w"){
		unique_lock<mutex> wlck(qwst); //Locks
		
		station_w.push(temp->train_num, temp->direction, temp->ldTime, temp->xTime);
		print_time(temp->train_num, 1, "West");
		
		while(station_w.is_prep() != 1 || station_w.peek_train() != temp->train_num){ //While first train is not rdy and this train is not next loop while
			cv_wst.wait(wlck);
		}
		station_w.pop(); //remove thread
		qwst.unlock(); //unlocks queue
		lck.lock(); //locks for crossing
		int cross = temp->xTime;
		print_time(temp->train_num, 2, "West");
		this_thread::sleep_for(chrono::milliseconds(cross*100)); //cross time
		print_time(temp->train_num, 3, "West");
		
		lck.unlock(); //release track
		cvT.notify_one(); //notify dispatcher
		
	}else if(temp->direction == "e"){
		unique_lock<mutex> elck(qest); //Locks
		station_e.push(temp->train_num, temp->direction, temp->ldTime, temp->xTime);
		print_time(temp->train_num, 1, "East");
		
		while(station_e.is_prep() != 1 || station_e.peek_train() != temp->train_num){ //While first train is not rdy and this train is not next loop while
			cv_est.wait(elck);
		}
		station_e.pop(); //remove thread
		qest.unlock(); //unlocks queue
		lck.lock(); //locks for crossing
		int cross = temp->xTime;
		print_time(temp->train_num, 2, "East");
		this_thread::sleep_for(chrono::milliseconds(cross*100)); //cross time
		print_time(temp->train_num, 3, "East");
		lck.unlock(); //release track
		cvT.notify_one(); //notify dispatcher

	}else{
		cout << " ERROR input file incorrect. \n";
	}
}

void dispatcher(){
	int train_complete = 0; //completed crossing
	int last_dir = 0; // last direction

	//ready state during peek.
	int train_at_W = 0;
	int train_at_E = 0;
	int train_at_w = 0;
	int train_at_e = 0;
	
	while(train_complete != number_of_trains){

		//Prevents change from 
		unique_lock<mutex> lck(track);
		unique_lock<mutex> Elck(qE);
		unique_lock<mutex> Wlck(qW);
		unique_lock<mutex> elck(qest);
		unique_lock<mutex> wlck(qwst);

		train_at_W = station_W.peek();
		train_at_E = station_E.peek();
		train_at_w = station_w.peek();
		train_at_e = station_e.peek();
		if(train_at_W == 1 || train_at_E == 1){ //one high has train.
			elck.unlock();
			wlck.unlock();
			if(train_at_W == 1 && train_at_E == 1){ //both high have train
				if(last_dir == 0){ //Last train was W/w or at start.
					Wlck.unlock(); //unused
					station_E.prep(); //node is ready
					Elck.unlock(); //free. might cause trouble here.
					cvE.notify_all();
					last_dir = 1;					
				}else{ //last train went was E/e
					Elck.unlock(); //unused
					station_W.prep(); //node is ready
					Wlck.unlock(); //free. might cause trouble here.
					cvW.notify_all();
					last_dir = 0;
				}
			}else if(train_at_E == 1){ //only exist high train at E
				Wlck.unlock(); //unused
				station_E.prep(); //node is ready
				Elck.unlock(); //free. might cause trouble here.
				cvE.notify_all();
				last_dir = 1;				
			}else{ //only exist high train at W
				Elck.unlock(); //unused
				station_W.prep(); //node is ready
				Wlck.unlock(); //free. might cause trouble here.
				cvW.notify_all();
				last_dir = 0;
			}
			cvT.wait(lck);
			lck.unlock();
			train_complete++;
		}else if(train_at_w == 1 || train_at_e == 1){
			Elck.unlock();
			Wlck.unlock();
			if(train_at_w == 1 && train_at_e == 1){ //both high have train
				if(last_dir == 0){ //Last train was W/w or at start.
					wlck.unlock(); //unused
					station_e.prep(); //node is ready
					elck.unlock(); //free. might cause trouble here.
					cv_est.notify_all();	
					last_dir = 1;
				}else{ //last train went was E/e
					elck.unlock(); //unused
					station_w.prep(); //node is ready
					wlck.unlock(); //free. might cause trouble here.
					cv_wst.notify_all();
					last_dir = 0;
				}
			}else if(train_at_e == 1){ //only exist high train at e
				wlck.unlock(); //unused
				station_e.prep(); //node is ready
				elck.unlock(); //free. might cause trouble here.
				cv_est.notify_all();
				last_dir = 1;				
			}else{ //only exist high train at w
				elck.unlock(); //unused
				station_w.prep(); //node is ready
				wlck.unlock(); //free. might cause trouble here.
				cv_wst.notify_all();
				last_dir = 0;
			}
			cvT.wait(lck);
			lck.unlock();
			train_complete++;
		}else{
			lck.unlock();
			elck.unlock();
			wlck.unlock();
			Elck.unlock();
			Wlck.unlock();
		}	
	}
}

int main (int argc, char *argv[]) {
	
	if(argc != 2){
		cout << "Requires ./mts.exe filename.txt \n";
		return 0;
	}
	
	string input;
	ifstream myfile (argv[1]);
	
	if (!myfile.is_open()){ //problem opening file.
		cout << "Unable to open file";
		return 0;
	}
    while (std::getline(myfile, input)){
        number_of_trains++;
	}
	//reset myfile
	myfile.clear();
	myfile.seekg(0);
	
	thread allTrains[number_of_trains];
	
	unique_lock<mutex> lck(track);//Locks for concurrent loading process
	int i = 0;
	while (myfile >> input){
		
		train_data *temp = new train_data;
		temp->train_num = i;
		temp->direction = input;
		myfile >> input;
		temp->ldTime = stoi(input);
		myfile >> input;
		temp->xTime = stoi(input);
		
		allTrains[i] = thread(chu_chu_train, temp);
		i++;
	}
	
	myfile.close();
	
	lck.unlock();
	this_thread::sleep_for(chrono::seconds(1)); //Time allowance for last thread to arrive at wait.
	timer = chrono::system_clock::now();//start clock.
	cvT.notify_all();
	
	thread dispatch(dispatcher);
	dispatch.join();
	
	for(int j=0; j<number_of_trains; j++){
		allTrains[j].join();
	}	
	return 0;
}