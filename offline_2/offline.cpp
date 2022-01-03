#include <iostream>
#include<cstdio>
#include<pthread.h>
#include<semaphore.h>
#include <unistd.h>
#include <chrono>
#include <algorithm>

using namespace std;

typedef void * (*THREADFUNCPTR)(void *);
auto init_time = chrono::steady_clock::now();

sem_t check_in_empty;
sem_t sec_check_empty;
sem_t belt_empty;
sem_t vip_channel_count;
sem_t LR_count;
sem_t RL_count;

pthread_mutex_t boarding_mutex;
pthread_mutex_t check_in_count_mutex;
pthread_mutex_t belts_count_mutex;
pthread_mutex_t VIP_LR_mutex;
pthread_mutex_t VIP_RL_mutex;
pthread_mutex_t VIP_waiting_mutex;
pthread_mutex_t VIP_priority_mutex;

int check_in_count = 0;
int belts_count = 0;
int VIP_LR_count = 0;
int VIP_RL_count = 0;
int VIP_LR_waiting = 0;


int get_idx(bool arr[], int n, bool elem)
{
	auto itr = find(arr, arr + n, elem);
 
    if (itr != (arr+n))
    {
        return distance(arr, itr);
    }
    else 
    {
        return -1;
    }
}


class Passenger
{
	int id;
	bool VIP;
	bool boarding_pass_lost;

	int nkiosks;
	int nbelts;
	int passenger_per_belt;

	int check_in_time;
	int security_check_time;
	int boarding_time;
	int vip_channel_time;

	bool* kiosks;
	bool* belts;
	
	public:

		Passenger(int id, bool vip, bool lost, int M, int N, int P, int c_time, int s_time, int b_time, int v_time, bool* kiosks, bool* belts)
		{
			this->id = id;
			this->VIP = vip;
			this->boarding_pass_lost = lost;

			this->nkiosks = M;
			this->nbelts = N;
			this->passenger_per_belt = P;

			this->check_in_time = c_time;
			this->security_check_time = s_time;
			this->boarding_time = b_time;
			this->vip_channel_time = v_time;

			this->kiosks = kiosks;
			this->belts = belts;
		}

		void * check_in()
		{
			int time;
			int assigned_kiosk;

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %d has arrived at the airport at time %d\n", id, time);
			fflush(stdout);

			// ---------------------------- critical region start ---------------------- //

			sem_wait(&check_in_empty);																					// down(empty) by 1
			
			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();



			// code inside this block has to be executed by only one thread at a time, so mutex is used //
			pthread_mutex_lock(&check_in_count_mutex);																	// locking the block
			
			check_in_count++;																							// increasing occupied kiosk count
			assigned_kiosk = get_idx(kiosks, nkiosks, false);															// getting the available kiosk index
			kiosks[assigned_kiosk] = true;																				// making the kiosk occupied
			
			printf("Passenger %d has started self-check in kiosk %d at time %d\n", id, assigned_kiosk+1, time);
			fflush(stdout);
			
			pthread_mutex_unlock(&check_in_count_mutex);																// unlocking the block
			// block ended //


			sleep(check_in_time);																						// executing the work

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();



			// code inside this block has to be executed by only one thread at a time, so mutex is used //
			pthread_mutex_lock(&check_in_count_mutex);																	// locking the block
			
			check_in_count--;																							// decreasing occupied kiosk count
			kiosks[assigned_kiosk] = false;																				// making the kiosk available again
			
			printf("Passenger %d has finished self-check in kiosk %d at time %d\n", id, assigned_kiosk+1, time);
			fflush(stdout);
			
			pthread_mutex_unlock(&check_in_count_mutex);																// unlocking the block
			// block ended //


			sem_post(&check_in_empty);																					// up(empty) by 1
			
			// ---------------------------- critical region end ---------------------- //
		}


		void * security_check()
		{
			int time;
			int assigned_belt;

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %d has started waiting for security check at time %d\n", id, time);

			// ---------------------------- critical region start ---------------------- //
			
			sem_wait(&belt_empty);																					// down(empty) by 1



			// code inside this block has to be executed by only one thread at a time, so mutex is used //
			pthread_mutex_lock(&belts_count_mutex);																	// locking the block
			
			belts_count++;																							// increasing occupied belt count
			assigned_belt = get_idx(belts, nbelts*passenger_per_belt, false);										// getting the available belt slot
			belts[assigned_belt] = true;																			// occupying the slot	
			
			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();		
			printf("Passenger %d has started security check in belt %d at time %d\n", id, (assigned_belt/passenger_per_belt)+1, time);

			if(belts_count%passenger_per_belt == 0)																	// if occupied belt_count has crossed passenger_per_belt
			{
				sem_wait(&sec_check_empty);																			// down(sec_check) by 1
			}
			
			pthread_mutex_unlock(&belts_count_mutex);																// unlocking the block
			// block ended //



			sleep(security_check_time);																				// executing the service

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %d has crossed the security check at time %d\n", id, time);


			// code inside this block has to be executed by only one thread at a time, so mutex is used //
			pthread_mutex_lock(&belts_count_mutex);																	// locking the block
			belts_count--;																							// decreasing the occupied belt count
			belts[assigned_belt] = false;																			// releasing the slot

			if((belts_count+1)%passenger_per_belt == 0)																// if occupied belt_count has crossed passenger_per_belt
			{
				sem_post(&sec_check_empty);																			// up(sec_check)
			}
			pthread_mutex_unlock(&belts_count_mutex);																// unlocking the block
			// block ended //


			sem_post(&belt_empty);																					// up(empty) by 1

			// ---------------------------- critical region end ---------------------- //
		}


		void * VIP_channel_LR()
		{
			int time;


			// code inside this block has to be executed by only one thread at a time, so mutex is used //
			pthread_mutex_lock(&VIP_waiting_mutex);																	// locking the block

			VIP_LR_waiting++;																						// waiting list for LR increased
			if(VIP_LR_waiting == 1)																					// if 1 person is waiting
				pthread_mutex_lock(&VIP_priority_mutex);															// lock the RL channel

			pthread_mutex_unlock(&VIP_waiting_mutex);																// unlocking the block
			// block ended //

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %d (VIP) has arrived at VIP channel at time %d\n", id, time);



			// code inside this block has to be executed by only one thread at a time, so mutex is used //
			pthread_mutex_lock(&VIP_LR_mutex);																		// locking the block

			VIP_LR_waiting--;																						// waiting person is now in channel
			if(VIP_LR_waiting == 0)																					// if no person is waiting
				pthread_mutex_unlock(&VIP_priority_mutex);															// unlock the RL channel

			VIP_LR_count++;																							// LR_channel count++
			if(VIP_LR_count == 1)																					// if 1 person is using LR channel
				pthread_mutex_lock(&VIP_RL_mutex);																	// lock the RL channel

			pthread_mutex_unlock(&VIP_LR_mutex);																	// unlocking the block
			// block ended //



			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %d (VIP) has started to cross VIP channel at time %d\n", id, time);

			sleep(vip_channel_time);																				// executing the work

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %d (VIP) has crossed the VIP channel at time %d\n", id, time);



			// code inside this block has to be executed by only one thread at a time, so mutex is used //
			pthread_mutex_lock(&VIP_LR_mutex);																		// locking the block
			
			VIP_LR_count--;																							// decreasing the LR_channel count
			if(VIP_LR_count == 0)																					// if no one is using the LR channel
				pthread_mutex_unlock(&VIP_RL_mutex);																// unlock the RL channel
			
			pthread_mutex_unlock(&VIP_LR_mutex);																	// unlocking the block
			// block ended //
		}


		void * VIP_channel_RL()
		{
			int time;

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %d is is waiting at VIP channel for the special kiosk at time %d\n", id, time);


			pthread_mutex_lock(&VIP_priority_mutex);				// if anyone is waiting at the LR_channel, lock the RL_channel (prioirity)
			pthread_mutex_unlock(&VIP_priority_mutex);


			// code inside this block has to be executed by only one thread at a time, so mutex is used //
			pthread_mutex_lock(&VIP_RL_mutex);																	// locking the block
			
			VIP_RL_count++;																						// VIP_RL count++
			if(VIP_RL_count==1)																					// if anyone is using the RL channel
				pthread_mutex_lock(&VIP_LR_mutex);																// lock the LR channel
			
			pthread_mutex_unlock(&VIP_RL_mutex);																// unlockng the block
			// block ended //


			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %d is heading for the special kiosk through VIP channel at time %d\n", id, time);
			
			sleep(vip_channel_time);																			// executing the work

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %d has re-recieved his boarding pass at time %d\n", id, time);



			// code inside this block has to be executed by only one thread at a time, so mutex is used //
			pthread_mutex_lock(&VIP_RL_mutex);																	// locking the block
			
			VIP_RL_count--;																						// decrease the RL_channel count
			if(VIP_RL_count==0)																					// if no one is using the RL channel
				pthread_mutex_unlock(&VIP_LR_mutex);															// unlock the LR_channel
			
			pthread_mutex_unlock(&VIP_RL_mutex);																// unlocking the block
			// block ended //
		}


		void * board()
		{
			int time;

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %d has started waiting to be boarded at time %d\n", id, time);

			// ---------------------------- critical region start ---------------------- //
			
			pthread_mutex_lock(&boarding_mutex);																// lock the critical region (one thread at a time)

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %d has started boarding the plane at time %d\n", id, time);

			sleep(boarding_time);																				// executing the work

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %d has boarded on the plane at time %d\n", id, time);	

			pthread_mutex_unlock(&boarding_mutex);																// unlock the critical region
			
			// ---------------------------- critical region end ---------------------- //
		}


		void * simulate()
		{
			// check_in();
			
			if(!VIP)
				security_check();
			else
				VIP_channel_LR();

			if(boarding_pass_lost)
			{
				printf("Passenger %d has lost his boarding pass\n", id);
				VIP_channel_RL();
			}
			
			// board();
		}

};


int main()
{
	int M = 3;
	int N = 2;
	int P = 2;

	int w = 3;
	int x = 3;
	int y = 3;
	int z = 3;

	// initializing the semaphores
	sem_init(&check_in_empty, 0, M);
	sem_init(&sec_check_empty, 0, N);
	sem_init(&belt_empty, 0, N*P);
	sem_init(&vip_channel_count, 0, 0);

	// initializing the mutexes
	pthread_mutex_init(&boarding_mutex, NULL);
	pthread_mutex_init(&check_in_count_mutex, NULL);
	pthread_mutex_init(&belts_count_mutex, NULL);
	pthread_mutex_init(&VIP_LR_mutex, NULL);
	pthread_mutex_init(&VIP_RL_mutex, NULL);
	pthread_mutex_init(&VIP_waiting_mutex, NULL);
	pthread_mutex_init(&VIP_priority_mutex, NULL);


	bool* kiosks;
	kiosks = new bool[M];

	for(int i=0; i<M; i++)
	{
		kiosks[i] = false;
	}


	bool* belts;
	belts = new bool[N*P];

	for(int i=0; i<N*P; i++)
	{
		belts[i] = false;
	}


	// generating the passengers
	for(int i=0; i<10; i++)
	{
		bool VIP = true;
		bool lost = false;
		
		if(i%2 == 0)
		{
			lost = true;
		}

		Passenger* p = new Passenger(i+1, VIP, lost, M, N, P, w, x, y, z, kiosks, belts);

		pthread_t thread;

		// Create thread using memeber function as startup routine
		pthread_create(&thread, NULL, (THREADFUNCPTR) &Passenger::simulate, p);

		sleep(int((10-i)/2));
	}

	pthread_exit(NULL);

	return 0;
}