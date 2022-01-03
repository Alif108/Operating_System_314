#include <iostream>
#include<cstdio>
#include<pthread.h>
#include<semaphore.h>
#include <unistd.h>
#include <chrono>

using namespace std;

typedef void * (*THREADFUNCPTR)(void *);
auto init_time = chrono::steady_clock::now();

sem_t check_in_full;
sem_t check_in_empty;

sem_t sec_check_full;
sem_t sec_check_empty;

sem_t belt_full;
sem_t belt_empty;

sem_t vip_channel_count;

sem_t LR_count;
sem_t RL_count;

pthread_mutex_t boarding_mutex;

pthread_mutex_t vip_LR_mutex;
pthread_mutex_t vip_RL_mutex;

// typedef struct vip_channel_priority
// {
// 	int LR_count = 0;
// 	int RL_count = 0;
// } vip_lock;


// class vip_channel_priority
// {
// 	// int LR_count = 0;
// 	// int RL_count = 0;

// 	void vip_LR()
// 	{
// 		sem_post(LR_count);

// 		// work

// 		sem_wait(LR_count);
// 	}
// };


class Passenger
{
	int id;
	bool VIP;
	bool boarding_pass_lost;

	int kiosks;
	int belts;
	int passenger_per_belt;

	int check_in_time;
	int security_check_time;
	int boarding_time;
	int vip_channel_time;
	
	public:

		Passenger(int id, bool vip, bool lost, int M, int N, int P, int c_time, int s_time, int b_time, int v_time)
		{
			this->id = id;
			this->VIP = vip;
			this->boarding_pass_lost = lost;

			this->kiosks = M;
			this->belts = N;
			this->passenger_per_belt = P;

			this->check_in_time = c_time;
			this->security_check_time = s_time;
			this->boarding_time = b_time;
			this->vip_channel_time = v_time;
		}

		int get_id()
		{
			return this->id;
		}

		bool is_VIP()
		{
			return this->VIP;
		}


		void * check_in()
		{
			int count;
			int time;

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %d has arrived at the airport at time %d\n", id, time);

			// -- critical region start -- //
			sem_wait(&check_in_empty);
			sem_post(&check_in_full);

			sem_getvalue(&check_in_full, &count);
			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %d has started self-check in kiosk %d at time %d\n", id, count, time);

			sleep(check_in_time);

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %d has has finished check at time %d\n", id, time);

			sem_wait(&check_in_full);
			sem_post(&check_in_empty);

			// -- critical region end -- //
		}


		void * security_check()
		{
			int belt_count;
			int time;

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %d has started waiting for security check at time %d\n", id, time);

			// -- critical region start -- //
			sem_wait(&belt_empty);
			sem_post(&belt_full);

			sem_getvalue(&belt_full, &belt_count);
			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %d has started security check in belt %d at time %d\n", id, ((belt_count-1)/passenger_per_belt)+1, time);

			if(belt_count%passenger_per_belt == 0)
			{
				sem_wait(&sec_check_empty);
				sem_post(&sec_check_full);
			}

			sleep(security_check_time);

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %d has crossed the security check at time %d\n", id, time);

			sem_wait(&belt_full);
			sem_post(&belt_empty);

			sem_getvalue(&belt_full, &belt_count);

			if((belt_count+1)%passenger_per_belt == 0)
			{
				sem_wait(&sec_check_full);
				sem_post(&sec_check_empty);
			}
			// -- critical region end -- //
		}


		void * VIP_channel_LR()
		{
			int time;
			int temp;

			sem_post(&vip_channel_count);

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %d (VIP) has arrived at VIP channel at time %d\n", id, time);

			sleep(vip_channel_time);

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %d (VIP) has crossed the VIP channel at time %d\n", id, time);

			sem_wait(&vip_channel_count);
		}


		void * VIP_channel_RL()
		{
			int time;
			int count;

			sem_getvalue(&vip_channel_count, &count);

			while(count>0)
			{
				sem_getvalue(&vip_channel_count, &count);
			}

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %d is heading for the special kiosk through VIP channel at time %d\n", id, time);
			
			sleep(vip_channel_time);

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %d has re-recieved his boarding pass at time %d\n", id, time);
		}


		void * board()
		{
			int time;

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %d has started waiting to be boarded at time %d\n", id, time);

			// -- critical region start -- //
			pthread_mutex_lock(&boarding_mutex);

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %d has started boarding the plane at time %d\n", id, time);

			sleep(boarding_time);

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %d has boarded on the plane at time %d\n", id, time);	

			pthread_mutex_unlock(&boarding_mutex);
			// -- critical region end -- //
		}


		void * simulate()
		{
			check_in();
			
			if(!VIP)
				security_check();
			else
				VIP_channel_LR();

			if(boarding_pass_lost)
			{
				printf("Passenger %d has lost his boarding pass\n", id);
				VIP_channel_RL();
			}
			
			board();
		}

};


// class Airport
// {
// 	int kiosk_count;
// 	int belt_count;
// 	int passenger_per_belt;

// 	int check_in_time;
// 	int security_check_time;
// 	int boarding_time;
// 	int vip_channel_time;

// public:

// 	Airport(int kiosk, int belt, int pass_per_belt, int c_time, int s_time, int b_time, int v_time)
// 	{
// 		this->kiosk_count = kiosk;
// 		this->belt_count = belt;
// 		this->passenger_per_belt = pass_per_belt;

// 		this->check_in_time = c_time;
// 		this->security_check_time = s_time;
// 		this->boarding_time = b_time;
// 		this->vip_channel_time = v_time;
// 	}

// 	void * check_in(void * arg)
// 	{
// 		int id = (Passenger*)arg.get_id();
// 		int count;
// 		int time;

// 		sem_wait(&check_in_empty);
// 		sem_post(&check_in_full);

// 		sem_getvalue(&check_in_full, &count);
// 		time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
// 		printf("Passenger %d has started waiting in kiosk %d at time %d\n", id, count, time);

// 		sleep(check_in_time);

// 		time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
// 		printf("Passenger %d has got his boarding pass at time %d\n", id, time);

// 		sem_wait(&check_in_full);
// 		sem_post(&check_in_empty);
// 	}
// };



int main()
{

	int M = 3;
	int N = 2;
	int P = 2;

	int w = 3;
	int x = 3;
	int y = 3;
	int z = 3;

	sem_init(&check_in_full, 0, 0);
	sem_init(&check_in_empty, 0, M);

	sem_init(&sec_check_full, 0, 0);
	sem_init(&sec_check_empty, 0, N);

	sem_init(&belt_full, 0, 0);
	sem_init(&belt_empty, 0, N*P);

	sem_init(&vip_channel_count, 0, 0);
	pthread_mutex_init(&boarding_mutex, NULL);

	// // Pointer to object of class Task
	// Passenger* p1 = new Passenger(1, false, M, N, P, w, x, y, z);
	// Passenger* p2 = new Passenger(2, false, M, N, P, w, x, y, z);
	// Passenger* p3 = new Passenger(3, false, M, N, P, w, x, y, z);
	
	// //Thread ID
	// pthread_t thread1;
	// pthread_t thread2;
	// pthread_t thread3;
	
	// // Create thread using memeber function as startup routine
	// pthread_create(&thread1, NULL, (THREADFUNCPTR) &Passenger::simulate, p1);
	// pthread_create(&thread2, NULL, (THREADFUNCPTR) &Passenger::simulate, p2);
	// pthread_create(&thread3, NULL, (THREADFUNCPTR) &Passenger::simulate, p3);

	for(int i=0; i<10; i++)
	{
		bool VIP = false;
		bool lost = false;
		
		if(i%6 == 0)
		{
			VIP = true;
			lost = true;
		}

		// Pointer to object of class Task
		Passenger* p = new Passenger(i+1, VIP, lost, M, N, P, w, x, y, z);

		//Thread ID
		pthread_t thread;

		// Create thread using memeber function as startup routine
		pthread_create(&thread, NULL, (THREADFUNCPTR) &Passenger::simulate, p);

		// sleep(i);
	}

	pthread_exit(NULL);

	return 0;
}