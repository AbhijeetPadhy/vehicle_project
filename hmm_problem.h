#include "stdafx.h"
#include "obs_seq_generator.h"
#include <windows.h>
#include <Math.h>
#include <cstdio>



#define N 5
#define M 32
#define MAX_T 150
#define TRAINING_UTTERANCES 10                  // Set this to more if number of utterances in default model is to be increased
#define TESTING_UTTERANCES 10
#define TRAINING 0
#define TESTING 1
#define DEFAULT_MODEL 0
#define CUSTOM_MODEL 1

int VAR_TRAINING_UTTERANCES = 10;
int NO_OF_ITEMS = 6;
char recorded_filename[300];
int current_item = 0;
int current_utterance = 0;
int current_model = DEFAULT_MODEL;
int is_custom_model_present = 0;

long double A[N+1][N+1];
long double B[N+1][M+1];
long double PI[N+1];
long double A_BAR[N+1][N+1];
long double B_BAR[N+1][M+1];
long double PI_BAR[N+1];
long int OBS_SEQ[TRAINING_UTTERANCES+1][MAX_T+1];
long double ALPHA[MAX_T+1][N+1];
long double BETA[MAX_T+1][N+1];
long double XAI[MAX_T+1][N+1][N+1];
long double GAMMA[MAX_T+1][N+1];
long double DELTA[N+1][MAX_T+1];
long double PSI[N+1][MAX_T+1];
long double P_star = 0;
int Q_star[MAX_T+1];
int T[TRAINING_UTTERANCES+1];

long double COLLECTION_A[TRAINING_UTTERANCES+1][N+1][N+1];
long double COLLECTION_B[TRAINING_UTTERANCES+1][N+1][M+1];
const char UNIVERSE_PATH[] = "personal_universe.txt";

int read_A(char file[]){
	FILE *fptr;
	char line[1024];
	char *ptr;
	int count = 0;

	// Open A_1.txt file 
	if ((fptr = fopen(file,"r")) == NULL){
		printf("Error! opening file");
		return(1);
	}

	count = 1;

	// Read the Universe.csv file line by line
	while (fgets(line, 1024, fptr)){
		char *token;
 
		// get the first token
		token = strtok(line, "\t");
   
		// walk through other tokens
		for(int i=1;i<=N;i++){
			A[count][i] = strtod(token, &ptr);
			token = strtok(NULL, "\t");
		}
		count++;
	}
	fclose(fptr);
	/*
	printf("A Matrix-----------------------\n");
	for(int a=1;a<=N;a++){
		for(int b = 1;b<=N;b++)
			printf("%g ", A[a][b]);
		printf("\n");
	}
	printf("\n");
	*/
}

int read_B(char file[]){
	FILE *fptr;
	char line[1024];
	char *ptr;
	int count = 0;

	// Open B_1.txt file 
	if ((fptr = fopen(file,"r")) == NULL){
		printf("Error! opening file");
		return(1);
	}

	count = 1;

	// Read the Universe.csv file line by line
	while (fgets(line, 1024, fptr) && count <= N){
		char *token;
 
		// get the first token
		token = strtok(line, "\t");
   
		// walk through other tokens
		for(int i=1;i<=M;i++){
			B[count][i] = strtod(token, &ptr);
			token = strtok(NULL, "\t");
		}
		count++;
	}
	fclose(fptr);
	/*
	printf("B Matrix-----------------------\n");
	for(int a=1;a<=N;a++){
		for(int b = 1;b<=M;b++)
			printf("%g ", B[a][b]);
		printf("\n");
	}
	*/
}

int read_PI(char file[]){
	FILE *fptr;
	char line[1024];
	char *ptr;
	int count = 0;

	// Open Pi_1.txt file 
	if ((fptr = fopen(file,"r")) == NULL){
		printf("Error! opening file");
		return(1);
	}


	// Read the Universe.csv file line by line
	fgets(line, 1024, fptr);
	char *token;
 
	// get the first token
	token = strtok(line, "\t");
   
	// walk through other tokens
	for(int i=1;i<=N;i++){
		PI[i] = strtod(token, &ptr);
		token = strtok(NULL, "\t");
	}
	
	fclose(fptr);
	/*
	printf("PI Vector-----------------------\n");
	for(int a=1;a<=N;a++){
		printf("%g ", PI[a]);
		printf("\n");
	}
	printf("\n");
	*/
}


int read_T(int digit, int choice, int model){
	FILE *fptr;
	char file[300];
	file[0] = '\0';
	if(choice == TRAINING && model == DEFAULT_MODEL)
		sprintf(file,"output/obs_seq/frame_no_%d.txt",digit);
	else if(choice == TRAINING && model == CUSTOM_MODEL)
		sprintf(file,"custom_model/obs_seq/frame_no_%d.txt",digit);
	else
		sprintf(file,"testing/obs_seq/frame_no_%d.txt",digit);
	if ((fptr = fopen(file,"r")) == NULL){
		printf("Error! opening file");
		return 1;
	}
	int count = 1;
	while((fscanf(fptr,"%d", &T[count])) != EOF){
		count++;
	}
	fclose(fptr);

	// print T array
	int utterance = VAR_TRAINING_UTTERANCES;
	if(choice == TESTING)
		utterance = TESTING_UTTERANCES;
	for(int i=1;i<=utterance;i++)
		printf("%d ", T[i]);
	printf("\n");
}

int read_OBS_SEQ(int digit, int choice, int model){
	if(read_T(digit, choice, model) == 1)
		return 1;;

	FILE *fptr;
	char file[300];
	char line[1024];
	char *ptr;
	int count = 0;

	// Open OBS_SEQ_1.txt file 
	file[0] = '\0';
	if(choice == TRAINING && model == DEFAULT_MODEL)
		sprintf(file,"output/obs_seq/obs_%d.txt",digit);
	else if(choice == TRAINING && model == CUSTOM_MODEL)
		sprintf(file,"custom_model/obs_seq/obs_%d.txt",digit);
	else
		sprintf(file,"testing/obs_seq/obs_%d.txt",digit);
	if ((fptr = fopen(file,"r")) == NULL){
		printf("Error! opening file");
		return(1);
	}

	count = 1;
	int utterance = VAR_TRAINING_UTTERANCES;
	if(choice == TESTING)
		utterance = TESTING_UTTERANCES;
	// Read the observation sequence file line by line
	while (fgets(line, 1024, fptr) && count <= utterance){
		char *token;
 
		// get the first token
		token = strtok(line, "\t");
   
		// walk through other tokens
		for(int i=1;i<=T[count];i++){
			OBS_SEQ[count][i] = strtod(token, &ptr);
			token = strtok(NULL, "\t");
		}
		count++;
	}
	fclose(fptr);

	/*
	printf("OBS_SEQ Matrix-----------------------\n");
	for(int a=1;a<=VAR_TRAINING_UTTERANCES;a++){
		for(int b = 1;b<=T[a];b++)
			printf("%ld ", OBS_SEQ[a][b]);
		printf("\n");
	}
	*/
}

long double forward_procedure(int z){
	//Step 1: Initialisation
	for(int i=1;i<=N;i++){
		ALPHA[1][i] = PI[i] * B[i][OBS_SEQ[z][1]];
	}

	//Step 2: Iteration
	for(int t=1;t<T[z];t++){
		for(int j=1;j<=N;j++){
			long double sum = 0;
			for(int i=1;i<=N;i++){
				sum += ALPHA[t][i] * A[i][j];
			}
			ALPHA[t+1][j] = sum * B[j][OBS_SEQ[z][t+1]];
		}
	}

	//Step 3: Termination
	long double probability = 0;
	for(int i=1;i<=N;i++){
		probability += ALPHA[T[z]][i];
	}
	/*
	printf("\n\n---------RESULT---------");
	printf("%g\n", probability);
	*/
	return probability;
}

void backward_procedure(int z){
	//Step 1: Initialisation
	for(int i=1;i<=N;i++){
		BETA[T[z]][i] = 1;
	}

	//Step 2: Iteration
	for(int t=T[z]-1;t>=1;t--){
		for(int i=1;i<=N;i++){
			long double sum = 0;
			for(int j=1;j<=N;j++){
				sum += A[i][j] * B[j][OBS_SEQ[z][t+1]] * BETA[t+1][j];
			}
			BETA[t][i] = sum;
		}
	}
	
	//printf("\n\n---BETA---\n");
	/*
	for(int i=1;i<T[z]+1;i++){
		for(int j=1;j<N+1;j++){
			printf("%g\t", BETA[i][j]);
		}
		printf("\n");
	}
	*/
}

void do_test(){
	char file[300];
	for(int i=1;i<=TESTING_UTTERANCES;i++){
		long double max_prob = 0, cur_prob = 0; 
		int index = -1;
		for(int j=0;j<NO_OF_ITEMS;j++){
			file[0] = '\0';
			sprintf(file,"output/lambda/%d/A_%d.txt",j,j);
			//printf("%s", file);
			read_A(file);

			file[0] = '\0';
			sprintf(file,"output/lambda/%d/B_%d.txt",j,j);
			read_B(file);

			file[0] = '\0';
			sprintf(file,"output/lambda/%d/PI_%d.txt",j,j);
			read_PI(file);
			cur_prob = forward_procedure(i);

			//printf("digit %d has probabiltiy = %g\n", j, cur_prob);
			if(cur_prob>max_prob){
				max_prob = cur_prob;
				index = j;
			}
		}
		printf("The digit is %d\n\n", index);
	}
}

void compute_XAI(int z){
	//printf("-------XAI------------\n");
	for(int t=1;t<=T[z]-1;t++){
		long double probability_of_obsseq_given_lambda = 0;

		for(int i=1;i<=N;i++){
			for(int j=1;j<=N;j++){
				probability_of_obsseq_given_lambda += ALPHA[t][i] * A[i][j] * B[j][OBS_SEQ[z][t+1]] * BETA[t+1][j];
			}
		}

		for(int i=1;i<=N;i++){
			for(int j=1;j<=N;j++){
				XAI[t][i][j] = (ALPHA[t][i] * A[i][j] * B[j][OBS_SEQ[z][t+1]] * BETA[t+1][j]) / probability_of_obsseq_given_lambda;
				if(t==1){
					;//printf("%g ", XAI[t][i][j]);
				}
			}
			if(t==1)
				;//printf("\n");
		}
	}
}

void compute_gamma(int z){
	//printf("-----gamma------\n");
	for(int t=1;t<=T[z];t++){
		long double sum = 0;
		for(int i=1;i<=N;i++){
			sum += ALPHA[t][i] * BETA[t][i];
		}
		for(int i=1;i<=N;i++){
			GAMMA[t][i] = (ALPHA[t][i] * BETA[t][i]) / sum;
			//printf("%g ",GAMMA[t][i]);
		}
		//printf("\n");
	}
}

void find_new_PI(int z){
	//printf("\n------New PI-------\n");
	for(int i=1;i<=N;i++){
		PI_BAR[i] = GAMMA[1][i];
		//printf("%g ", PI_BAR[i]);
	}
	//printf("\n");
}

void find_new_A(int z){
	//printf("\n------New A-------\n");
	for(int i=1;i<=N;i++){
		for(int j=1;j<=N;j++){
			long double num = 0, den = 0;
			for(int t=1;t<=T[z]-1;t++){
				num += XAI[t][i][j];
				den += GAMMA[t][i];
				//printf("My gamma is %g and t is %d and i is %d \n", GAMMA[t][i],t,i);
			}
			A_BAR[i][j] = num/den;
			//printf("%g ", A_BAR[i][j]);
		}
		//printf("\n");
	}
	
	// refine A_BAR
	for(int j=1;j<=N;j++){
		long double row_sum = 0;
		int max_index = 0;
		for(int k=1;k<=N;k++){
			if(A_BAR[j][k] > A_BAR[j][max_index])
				max_index = k;
			row_sum += A_BAR[j][k];
		}
		A_BAR[j][max_index] -= (row_sum-1); 
	}
}

void find_new_B(int z){
	//printf("-----B_BAR----------\n");
	for(int j=1;j<=N;j++){
		for(int k=1;k<=M;k++){
			long double num = 0, den = 0;
			for(int t=1; t<=T[z]; t++){
				if(OBS_SEQ[z][t] == k)
					num += GAMMA[t][j];
				den += GAMMA[t][j];
			}
			B_BAR[j][k] = num/den;
		}
	}

	// refine B_BAR
	for(int j=1;j<=N;j++){
		long double row_sum = 0;
		int max_index = 0;
		for(int k=1;k<=M;k++){
			if(B_BAR[j][k] == 0)
				B_BAR[j][k] = 1e-030;
			if(B_BAR[j][k] > B_BAR[j][max_index])
				max_index = k;
			row_sum += B_BAR[j][k];
		}
		B_BAR[j][max_index] -= (row_sum-1); 
	}
}

void initialise_bakis_model(){

	//Initialise PI Matrix
	for(int i=1;i<=N;i++)
		PI[i] = 0;
	PI[1] = 1;

	// Initialise A Matrix
	for(int i=1;i<=N;i++){
		for(int j=1;j<=N;j++){
			A[i][j] = 0;
		}
	}
	A[1][1] = 0.8; A[1][2] = 0.2;
	A[2][2] = 0.8; A[2][3] = 0.2;
	A[3][3] = 0.8; A[3][4] = 0.2;
	A[4][4] = 0.8; A[4][5] = 0.2;
	A[5][5] = 1;

	// Initialise B Matrix
	for(int i=1;i<=N;i++){
		for(int j=1;j<=M;j++)
			B[i][j] = 0.03125;
	}
}

void viterbi_algorithm(int z){
	//Step 1: Initialisation
	for(int i=1;i<=N;i++){
		DELTA[i][1] = PI_BAR[i] * B_BAR[i][OBS_SEQ[z][1]];
		PSI[i][1] = 0;
	}

	//Step 2: Iteration
	for(int t=2;t<=T[z];t++){
		for(int j=1;j<=N;j++){
			long double max = DELTA[1][t-1] * A_BAR[1][j];
			int index = 1;
			for(int i=1;i<=N;i++){
				if(DELTA[i][t-1] * A_BAR[i][j] > max){
					max = DELTA[i][t-1] * A_BAR[i][j];
					index = i;
				}
			}
			DELTA[j][t] = max * B_BAR[j][OBS_SEQ[z][t]];
			PSI[j][t] = index;
		}
	}

	//Step 3: Termination
	long double max = DELTA[1][T[z]];
	int index = 1;
	for(int i=1;i<=N;i++){
		if(DELTA[i][T[z]] > max){
			max = DELTA[i][T[z]];
			index = i;
		}
	}
	P_star = max;
	Q_star[T[z]] = index;

	//Step 4: State Sequence Path Back Tracking
	for(int t=T[z]-1;t>0;t--){
		Q_star[t] = PSI[Q_star[t+1]][t+1];
	}

	//printf("\n---------RESULT---------");
	//printf("%g\n", P_star);
	for(int i=1;i<=T[z];i++){
		//printf("%d ", Q_star[i]);
	}
}

void update_lambda(){
	for(int i=1;i<=N;i++){
		for(int j=1;j<=N;j++)
			A[i][j] = A_BAR[i][j];
	}
	for(int i=1;i<=N;i++){
		for(int j=1;j<=M;j++)
			B[i][j] = B_BAR[i][j];
	}
}

void print_lambda(){
	printf("PI Vector \n");
	for(int i=1;i<=N;i++){
		printf("%g\t", PI[i]);
	}
	printf("\n");

	printf("A Matrix \n");
	for(int i=1;i<=N;i++){
		for(int j=1;j<=N;j++){
			printf("%g\t", A[i][j]);
		}
		printf("\n");
	}
	printf("\n");

	printf("B Matrix \n");
	for(int i=1;i<=N;i++){
		for(int j=1;j<=M;j++){
			printf("%g\t", B[i][j]);
		}
		printf("\n");
	}
	printf("\n");
}

void add_lambda_to_database(int utterance){
	for(int i=1;i<=N;i++){
		for(int j=1;j<=N;j++){
			COLLECTION_A[utterance][i][j] = A[i][j];
		}
	}
	for(int i=1;i<=N;i++){
		for(int j=1;j<=M;j++){
			COLLECTION_B[utterance][i][j] = B[i][j];
		}
	}
}

void average_lambda(){
	//Average of A
	for(int i=1;i<=N;i++){
		for(int j=1;j<=N;j++){
			long double sum = 0;
			for(int utterance = 1 ; utterance <= VAR_TRAINING_UTTERANCES ; utterance++){
				sum += COLLECTION_A[utterance][i][j];
			}
			sum = sum / VAR_TRAINING_UTTERANCES;
			A[i][j] = sum;
		}
	}


	//Average of B
	for(int i=1;i<=N;i++){
		for(int j=1;j<=M;j++){
			long double sum = 0;
			for(int utterance = 1 ; utterance <= VAR_TRAINING_UTTERANCES ; utterance++){
				sum += COLLECTION_B[utterance][i][j];
			}
			sum = sum / VAR_TRAINING_UTTERANCES;
			B[i][j] = sum;
		}
	}
}

void output_lambda_to_file(int digit, int model){
	FILE *fptr;
	char filename[300];

	// output PI
	filename[0] = '\0';
	if(model == DEFAULT_MODEL)
		sprintf(filename,"output/lambda/%d/PI_%d.txt",digit,digit);
	else if(model == CUSTOM_MODEL){
		sprintf(filename,"custom_model/lambda/%d/PI_%d.txt",digit,digit);
	}
	if ((fptr = fopen(filename,"w")) == NULL){
			printf("Error! opening file");
		}
	for(int i=1;i<=N;i++){
		fprintf(fptr, "%g\t", PI[i]);
	}
	fclose(fptr);

	// output A
	filename[0] = '\0';
	if(model == DEFAULT_MODEL)
		sprintf(filename,"output/lambda/%d/A_%d.txt",digit,digit);
	else if(model == CUSTOM_MODEL){
		sprintf(filename,"custom_model/lambda/%d/A_%d.txt",digit,digit);
	}
	if ((fptr = fopen(filename,"w")) == NULL){
			printf("Error! opening file");
		}
	for(int i=1;i<=N;i++){
		for(int j=1;j<=N;j++)
			fprintf(fptr, "%g\t", A[i][j]);
		fprintf(fptr, "\n");
	}
	fclose(fptr);

	// output B
	filename[0] = '\0';
	if(model == DEFAULT_MODEL)
		sprintf(filename,"output/lambda/%d/B_%d.txt",digit,digit);
	else if(model == CUSTOM_MODEL)
		sprintf(filename,"custom_model/lambda/%d/B_%d.txt",digit,digit);
	if ((fptr = fopen(filename,"w")) == NULL){
			printf("Error! opening file");
		}
	for(int i=1;i<=N;i++){
		for(int j=1;j<=M;j++)
			fprintf(fptr, "%g\t", B[i][j]);
		fprintf(fptr, "\n");
	}
	fclose(fptr);
}

void converge(int digit, int model){
	printf("Converging digit %d\n",digit);
	read_OBS_SEQ(digit, TRAINING, model);
	
	for(int j=1;j<=VAR_TRAINING_UTTERANCES;j++){
		initialise_bakis_model();
		printf("Observation Sequence: %d \n",j);
		for(int i=0;i<200;i++){
			forward_procedure(j);
			backward_procedure(j);
			compute_XAI(j);
			compute_gamma(j);
			find_new_PI(j);
			find_new_A(j);
			find_new_B(j);
			viterbi_algorithm(j);
			update_lambda();
			//printf("STEP %d: Probability of state sequence is : %g\n",i, P_star);
		}
		printf("Final Probability of state sequence is : %g\n", P_star);
		//print_lambda();
		add_lambda_to_database(j);
	}
	average_lambda();

	//print final lambda
	printf("\n\n The final lambda is \n");
	print_lambda();
	output_lambda_to_file(digit, model);
	
}

int generate_observation_sequence(int choice, int model){
	
	FILE *fptr, *fptr2, *fptr3 = NULL;
	char filename[300], filename2[300];
	
	int utterance = VAR_TRAINING_UTTERANCES;
	if(choice == TESTING)
		utterance = TESTING_UTTERANCES;

	if(choice == TRAINING){
		if ((fptr3 = fopen(UNIVERSE_PATH,"w")) == NULL){
			printf("Error! opening file");
		}
	}

	int frames_per_oberservation[TRAINING_UTTERANCES];
	for(int i=0;i<VAR_TRAINING_UTTERANCES;i++)
		frames_per_oberservation[i] = 0;

	// Read code book
	if(read_codebook())
		return 1;
	
	// Produce C values of all the recordings
	// for all the digits
	for(int i=0;i<NO_OF_ITEMS;i++){
		filename[0] = '\0';filename2[0] = '\0';
		if(choice == TRAINING && model == DEFAULT_MODEL){
			sprintf(filename,"output/obs_seq/obs_%d.txt",i);
			sprintf(filename2,"output/obs_seq/frame_no_%d.txt",i);
		}else if(choice == TRAINING && model == CUSTOM_MODEL){
			sprintf(filename,"custom_model/obs_seq/obs_%d.txt",i);
			sprintf(filename2,"custom_model/obs_seq/frame_no_%d.txt",i);
		}else{
			sprintf(filename,"testing/obs_seq/obs_%d.txt",i);
			sprintf(filename2,"testing/obs_seq/frame_no_%d.txt",i);
		}

		if ((fptr = fopen(filename,"w")) == NULL){
			printf("Error! opening file");
			return 1;
		}
		if ((fptr2 = fopen(filename2,"w")) == NULL){
			printf("Error! opening file");
			return 1;
		}
		
		// for all the utterances
		for(int j=0;j<utterance;j++){
			frames_per_oberservation[j] = extract_stable_frame_data(i,j, choice, model);
			populate_C(frames_per_oberservation[j], fptr3);
			produce_observation_sequence(frames_per_oberservation[j]);
			for(int k=0;k<frames_per_oberservation[j];k++){
				fprintf(fptr, "%d\t", OBS_SEQ_GEN[k]);
			}
			fprintf(fptr, "\n");
			fprintf(fptr2, "%d\n", frames_per_oberservation[j]);
		}
		
		fclose(fptr);
		fclose(fptr2);
	}
	if(fptr3 != NULL)
		fclose(fptr3);
	return 0;
}

void record(){
	if(current_item <= NO_OF_ITEMS-1 && current_utterance <= VAR_TRAINING_UTTERANCES-1){
		recorded_filename[0] = '\0';
		sprintf(recorded_filename, "custom_model/recordings/%d/obs_%d.wav", current_item, current_utterance+1);
		char command[300];
		command[0] = '\0';
		sprintf(command, "Recording_Module.exe 3 custom_model/recordings/%d/obs_%d.wav custom_model/recordings/%d/obs_%d.txt", current_item, current_utterance+1, current_item, current_utterance+1);
		system(command);
	}
	
}