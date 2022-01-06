#include<cstdio>
#include<pthread.h>
#include<semaphore.h>
#include <unistd.h>
#include <chrono>
#include <random>
#include <vector>
#include "helper.h"

#define TOTAL_TIME 40													// total time of generating passengers
#define ARRIVAL_RATE 5													// poisson arrival rate

using namespace std;


typedef void * (*THREADFUNCPTR)(void *);
auto init_time = chrono::steady_clock::now();							// taking the initial time

sem_t check_in_empty;													// counting semaphore for check_in kiosks (M)
sem_t sec_check_empty;													// counting semaphore for belts (N)
sem_t belt_empty;														// counting semaphore for total belts (N*P)

pthread_mutex_t boarding_mutex;
pthread_mutex_t check_in_count_mutex;
pthread_mutex_t belts_count_mutex;
pthread_mutex_t VIP_LR_mutex;
pthread_mutex_t VIP_RL_mutex;
pthread_mutex_t VIP_waiting_mutex;
pthread_mutex_t VIP_priority_mutex;
pthread_mutex_t special_kiosk_mutex;

int check_in_count = 0;
int belts_count = 0;
int VIP_LR_count = 0;
int VIP_RL_count = 0;
int VIP_LR_waiting = 0;

FILE *output_file;



// ------------ Passenger Class which will simulate the scenario ---------------- //
class Passenger
{
	int id;
	char* name;
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

		Passenger(int id, bool vip, int M, int N, int P, int c_time, int s_time, int b_time, int v_time, bool* kiosks, bool* belts)
		{
			this->id = id;
			this->VIP = vip;
			this->boarding_pass_lost = false;

			this->nkiosks = M;
			this->nbelts = N;
			this->passenger_per_belt = P;

			this->check_in_time = c_time;
			this->security_check_time = s_time;
			this->boarding_time = b_time;
			this->vip_channel_time = v_time;

			this->kiosks = kiosks;
			this->belts = belts;

			this->name = get_passenger_name(id, VIP);					// get_passenger_name() is defined in "helper.h"
		}

		void * check_in()
		{
			int time;
			int assigned_kiosk;

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %s has arrived at the airport at time %d\n", name, time);
			fflush(stdout);
			fprintf(output_file, "Passenger %s has arrived at the airport at time %d\n", name, time);
			fflush(output_file);

			// ---------------------------- critical region start ---------------------- //

			sem_wait(&check_in_empty);																					// down(empty) by 1

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();



			// code inside this block has to be executed by only one thread at a time, so mutex is used //
			pthread_mutex_lock(&check_in_count_mutex);																	// locking the block
			
			check_in_count++;																							// increasing occupied kiosk count
			assigned_kiosk = get_idx(kiosks, nkiosks, false);															// getting the available kiosk index
			kiosks[assigned_kiosk] = true;																				// making the kiosk occupied
			
			printf("Passenger %s has started self-check in kiosk %d at time %d\n", name, assigned_kiosk+1, time);
			fflush(stdout);
			fprintf(output_file, "Passenger %s has started self-check in kiosk %d at time %d\n", name, assigned_kiosk+1, time);
			fflush(output_file);
			
			pthread_mutex_unlock(&check_in_count_mutex);																// unlocking the block
			// block ended //


			sleep(check_in_time);																						// executing the work

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();



			// code inside this block has to be executed by only one thread at a time, so mutex is used //
			pthread_mutex_lock(&check_in_count_mutex);																	// locking the block
			
			check_in_count--;																							// decreasing occupied kiosk count
			kiosks[assigned_kiosk] = false;																				// making the kiosk available again
			
			printf("Passenger %s has finished self-check in kiosk %d at time %d\n", name, assigned_kiosk+1, time);
			fflush(stdout);
			fprintf(output_file, "Passenger %s has finished self-check in kiosk %d at time %d\n", name, assigned_kiosk+1, time);
			fflush(output_file);
			
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
			printf("Passenger %s has started waiting for security check at time %d\n", name, time);
			fflush(stdout);
			fprintf(output_file, "Passenger %s has started waiting for security check at time %d\n", name, time);
			fflush(output_file);

			// ---------------------------- critical region start ---------------------- //
			
			sem_wait(&belt_empty);																					// down(empty) by 1



			// code inside this block has to be executed by only one thread at a time, so mutex is used //
			pthread_mutex_lock(&belts_count_mutex);																	// locking the block
			
			belts_count++;																							// increasing occupied belt count
			assigned_belt = get_idx(belts, nbelts*passenger_per_belt, false);										// getting the available belt slot
			belts[assigned_belt] = true;																			// occupying the slot	
			
			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();		
			printf("Passenger %s has started security check in belt %d at time %d\n", name, (assigned_belt/passenger_per_belt)+1, time);
			fflush(stdout);
			fprintf(output_file, "Passenger %s has started security check in belt %d at time %d\n", name, (assigned_belt/passenger_per_belt)+1, time);
			fflush(output_file);

			if(belts_count%passenger_per_belt == 0)																	// if occupied belt_count has crossed passenger_per_belt
			{
				sem_wait(&sec_check_empty);																			// down(sec_check) by 1
			}
			
			pthread_mutex_unlock(&belts_count_mutex);																// unlocking the block
			// block ended //



			sleep(security_check_time);																				// executing the service

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %s has crossed the security check at time %d\n", name, time);
			fflush(stdout);
			fprintf(output_file, "Passenger %s has crossed the security check at time %d\n", name, time);
			fflush(output_file);


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

		void * special_kiosk()
		{
			int time;

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %s has started waiting in special kiosk at time %d\n", name, time);
			fflush(stdout);
			fprintf(output_file, "Passenger %s has started waiting in special kiosk at time %d\n", name, time);
			fflush(output_file);

			// ---------------------------- critical region start ---------------------- //
			
			pthread_mutex_lock(&special_kiosk_mutex);															// lock the critical region (one thread at a time)

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %s has entered special kiosk at time %d\n", name, time);
			fflush(stdout);
			fprintf(output_file, "Passenger %s has entered special kiosk at time %d\n", name, time);
			fflush(output_file);

			sleep(check_in_time);																				// executing the work

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %s has re-received boarding pass at time %d\n", name, time);	
			fflush(stdout);
			fprintf(output_file, "Passenger %s has re-received boarding pass at time %d\n", name, time);
			fflush(output_file);

			pthread_mutex_unlock(&special_kiosk_mutex);															// unlock the critical region
			
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
			printf("Passenger %s has arrived at VIP channel at time %d\n", name, time);
			fflush(stdout);
			fprintf(output_file, "Passenger %s has arrived at VIP channel at time %d\n", name, time);
			fflush(output_file);



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
			printf("Passenger %s has started to cross VIP channel at time %d\n", name, time);
			fflush(stdout);
			fprintf(output_file, "Passenger %s has started to cross VIP channel at time %d\n", name, time);
			fflush(output_file);

			sleep(vip_channel_time);																				// executing the work

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %s has crossed the VIP channel at time %d\n", name, time);
			fflush(stdout);
			fprintf(output_file, "Passenger %s has crossed the VIP channel at time %d\n", name, time);
			fflush(output_file);



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
			printf("Passenger %s is waiting at VIP channel for the special kiosk at time %d\n", name, time);
			fflush(stdout);
			fprintf(output_file, "Passenger %s is waiting at VIP channel for the special kiosk at time %d\n", name, time);
			fflush(output_file);


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
			printf("Passenger %s is heading for the special kiosk through VIP channel at time %d\n", name, time);
			fflush(stdout);
			fprintf(output_file, "Passenger %s is heading for the special kiosk through VIP channel at time %d\n", name, time);
			fflush(output_file);
			
			sleep(vip_channel_time);																			// executing the work

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %s has reached special kiosk at time %d\n", name, time);
			fflush(stdout);
			fprintf(output_file, "Passenger %s has reached special kiosk at time %d\n", name, time);
			fflush(output_file);



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
			printf("Passenger %s has started waiting to be boarded at time %d\n", name, time);
			fflush(stdout);
			fprintf(output_file, "Passenger %s has started waiting to be boarded at time %d\n", name, time);
			fflush(output_file);

			// ---------------------------- critical region start ---------------------- //
			
			pthread_mutex_lock(&boarding_mutex);													// lock the critical region (one thread at a time)

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %s has started boarding the plane at time %d\n", name, time);
			fflush(stdout);
			fprintf(output_file, "Passenger %s has started boarding the plane at time %d\n", name, time);
			fflush(output_file);

			sleep(boarding_time);																	// executing the work

			time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
			printf("Passenger %s has boarded on the plane at time %d\n", name, time);	
			fflush(stdout);
			fprintf(output_file, "Passenger %s has boarded on the plane at time %d\n", name, time);
			fflush(output_file);


			pthread_mutex_unlock(&boarding_mutex);													// unlock the critical region
			
			// ---------------------------- critical region end ---------------------- //
		}


		void * simulate()
		{
			check_in();
			sleep(1);
			
			if(VIP)
			{
				VIP_channel_LR();
				sleep(1);
			}
			else
			{
				security_check();
				sleep(1);
			}

			if(rand()%(rand()%10 + 1) == 0)					// randomly losing boarding pass
				boarding_pass_lost = true;

			while(boarding_pass_lost)
			{
				printf("Passenger %s has lost his boarding pass\n", name);
				fflush(stdout);
				fprintf(output_file, "Passenger %s has lost his boarding pass\n", name);
				fflush(output_file);


				VIP_channel_RL();
				sleep(1);

				special_kiosk();
				sleep(1);

				VIP_channel_LR();
				sleep(1);

				if(rand()%(rand()%10 + 1) == 0)				// randomly losing boarding pass
					boarding_pass_lost = true;
				else
					boarding_pass_lost = false;
			}

			board();

			pthread_exit(NULL);
		}

		~Passenger()
		{
			printf("Passenger %d has been deleted\n", id);
			fflush(stdout);
		}
};


// -------- Driver ---------- //

int main(int argc,char *argv[])
{

	if(argc!=2)
	{
		printf("Please provide input file name and try again\n");
		return 0;
	}

	int* params = get_params(argv[1]);								// get_params() defined in "helper.h"

	if(*params == -1)
	{
		printf("Cannot open specified file\n");
		return 0;
	}


	// -------------------- setting the parameters ------------------------ //

	int M = params[0];				// kiosk number
	int N = params[1];				// belt number
	int P = params[2];				// passenger per belt

	int w = params[3];				// time for self check-in at a kiosk
	int x = params[4];				// time for security check
	int y = params[5];				// time for boarding at the gate
	int z = params[6];				// time for walking on VIP channel in either direction


	
	// ------------------- initializing output file ---------------------- //

	output_file = fopen("output.txt","a");
	if(output_file == NULL)
	{
		printf("Cannot create output file\n");   
		return 0;
	}


	// ------------------- initializing the semaphores --------------------- //

	int return_value;
	
	return_value = sem_init(&check_in_empty, 0, M);					// initialized with number_of_kiosks
	if(return_value != 0)
	{
		printf("check_in_empty semaphore initializaion failed\n");
		return 0;
	}

	return_value = sem_init(&sec_check_empty, 0, N);				// initialized with number_of_belts
	if(return_value != 0)
	{
		printf("sec_check_empty semaphore initializaion failed\n");
		return 0;
	}

	return_value = sem_init(&belt_empty, 0, N*P);					// initialized with (number_of_belts * passenger_per_belt)
	if(return_value != 0)
	{
		printf("belt_empty semaphore initializaion failed\n");
		return 0;
	}


	// ------------------ initializing the mutexes --------------------- //

	return_value = pthread_mutex_init(&boarding_mutex, NULL);
	if(return_value != 0)
	{
		printf("boarding_mutex initializaion failed\n");
		return 0;
	}

	return_value = pthread_mutex_init(&check_in_count_mutex, NULL);
	if(return_value != 0)
	{
		printf("check_in_count_mutex initializaion failed\n");
		return 0;
	}
	
	return_value = pthread_mutex_init(&belts_count_mutex, NULL);
	if(return_value != 0)
	{
		printf("belts_count_mutex initializaion failed\n");
		return 0;
	}
	
	return_value = pthread_mutex_init(&VIP_LR_mutex, NULL);
	if(return_value != 0)
	{
		printf("VIP_LR_mutex initializaion failed\n");
		return 0;
	}
	
	return_value = pthread_mutex_init(&VIP_RL_mutex, NULL);
	if(return_value != 0)
	{
		printf("VIP_RL_mutex initializaion failed\n");
		return 0;
	}
	
	return_value = pthread_mutex_init(&VIP_waiting_mutex, NULL);
	if(return_value != 0)
	{
		printf("VIP_waiting_mutex initializaion failed\n");
		return 0;
	}
	
	return_value = pthread_mutex_init(&VIP_priority_mutex, NULL);
	if(return_value != 0)
	{
		printf("VIP_priority_mutex initializaion failed\n");
		return 0;
	}
	
	return_value = pthread_mutex_init(&special_kiosk_mutex, NULL);
	if(return_value != 0)
	{
		printf("special_kiosk_mutex initializaion failed\n");
		return 0;
	}


	// ---------- initializing other resources: kiosks and belts ---------------- //

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



	// ----------- generating the passengers in poisson distribution ------------- //

	unsigned seed = chrono::system_clock::now().time_since_epoch().count();
	default_random_engine generator (seed);
	srand(seed);

	int time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
	int id = 1;
	bool VIP;
	poisson_distribution<int> distribution (ARRIVAL_RATE);
	vector<Passenger*> passenger_vector;
	vector<pthread_t> thread_vector;

	while(time < TOTAL_TIME)
	{
		VIP = false;
	
		if(rand()%(rand()%10 + 1) == 0)							// randomly assigning VIP
			VIP = true;

		Passenger* p = new Passenger(id, VIP, M, N, P, w, x, y, z, kiosks, belts);
		pthread_t thread;

		passenger_vector.push_back(p);
		thread_vector.push_back(thread);

		// Creating thread using member function as startup routine
		return_value = pthread_create(&thread_vector.back(), NULL, (THREADFUNCPTR) &Passenger::simulate, p);		
		if(return_value != 0)
		{
			printf("Passenger %d thread could not be created\n", id);
			fflush(stdout);
		}

		time = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - init_time).count();
		id++;

		sleep(distribution(generator));
	}


	// --------------------- waiting until all the threads end --------------- //

	for (int i=0; i<thread_vector.size(); i++)
	{
		if(pthread_join(thread_vector[i], NULL))
			printf("Joining failed %d\n", x);
	}

	

	// --------------------- deallocating all the passengers ---------------- //

	printf("\n");
	for(int i=0; i<passenger_vector.size(); i++)
	{
		delete passenger_vector[i];
	}


	// ---------------- destroying the semaphores ----------------------//

	return_value = sem_destroy(&check_in_empty);
	if(return_value != 0)
	{
		printf("check_in_empty semaphore destroy failed\n");
		return 0;
	}

	return_value = sem_destroy(&sec_check_empty);
	if(return_value != 0)
	{
		printf("sec_check_empty semaphore destroy failed\n");
		return 0;
	}

	return_value = sem_destroy(&belt_empty);
	if(return_value != 0)
	{
		printf("belt_empty semaphore destroy failed\n");
		return 0;
	}


	// ------------------ destroying the mutexes ------------------------- //

	return_value = pthread_mutex_destroy(&boarding_mutex);
	if(return_value != 0)
	{
		printf("boarding_mutex destroy failed\n");
		return 0;
	}

	return_value = pthread_mutex_destroy(&check_in_count_mutex);
	if(return_value != 0)
	{
		printf("check_in_count_mutex destroy failed\n");
		return 0;
	}
	
	return_value = pthread_mutex_destroy(&belts_count_mutex);
	if(return_value != 0)
	{
		printf("belts_count_mutex destroy failed\n");
		return 0;
	}
	
	return_value = pthread_mutex_destroy(&VIP_LR_mutex);
	if(return_value != 0)
	{
		printf("VIP_LR_mutex destroy failed\n");
		return 0;
	}
	
	return_value = pthread_mutex_destroy(&VIP_RL_mutex);
	if(return_value != 0)
	{
		printf("VIP_RL_mutex destroy failed\n");
		return 0;
	}
	
	return_value = pthread_mutex_destroy(&VIP_waiting_mutex);
	if(return_value != 0)
	{
		printf("VIP_waiting_mutex destroy failed\n");
		return 0;
	}
	
	return_value = pthread_mutex_destroy(&VIP_priority_mutex);
	if(return_value != 0)
	{
		printf("VIP_priority_mutex destroy failed\n");
		return 0;
	}
	
	return_value = pthread_mutex_destroy(&special_kiosk_mutex);
	if(return_value != 0)
	{
		printf("special_kiosk_mutex destroy failed\n");
		return 0;
	}


	// ---------------- deallocating other resources ------------------- //

	delete[] kiosks;
	delete[] belts;

	// ---------------- closing file stream ---------------- //
	fclose(output_file);

	pthread_exit(NULL);

	return 0;
}