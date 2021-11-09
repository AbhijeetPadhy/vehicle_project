#include "stdafx.h"
#include <windows.h>
#include <Math.h>
#include <cstdio>

#define SAMPLES_IN_FRAME 320
#define FRAMES 150
#define STABLE_FRAME_FACTOR 0.005
#define P 12
#define TRAINING 0
#define TESTING 1
#define DEFAULT_MODEL 0
#define CUSTOM_MODEL 1

double frame_data[SAMPLES_IN_FRAME*FRAMES];		// stores samples of speech frames of a recording of a digit
double test_data[SAMPLES_IN_FRAME];				// Stores samples of a frame

const char INPUT_PATH[] = "input_file.txt";
const char OUTPUT_PATH[] = "output_file.txt";
const char OUTPUT_C[] = "output_c.txt";

// Arrays to store the computer A, R, C
double R[P+1], C[FRAMES][P+1], AA[P];

// Code Book
double codebook[32][12];

// Tokhura's Weights
int w[] = {1,3,7,13,19,22,25,33,42,50,56,61};

int OBS_SEQ_GEN[FRAMES];



// Function to Calculate R vector for a frame of 320 samples
void calculate_R(){
	for(int i=0;i<P+1;i++){
		R[i] = 0;
		for(int j = 0; j < SAMPLES_IN_FRAME-i ; j++){
			R[i] += test_data[j] * test_data[i+j];
		}
	}
}



// Function to Calculate A vector for a frame of 320 samples
void calculate_A(){
	double E = R[0], sum;
	double A_prev[P];
	double k = 0;
	for(int i=0;i<P;i++)
		A_prev[i] = 0;
	for(int i=1;i<=P;i++){
		sum = 0;
		for(int j = 1; j <= i-1 ; j++){
			sum += A_prev[j-1] * R[i-j];
		}
		k = (R[i] - sum)/E;
		AA[i-1] = k;
		E = (1-k*k)*E;
		for(int j = 1; j <= i-1; j++)
			AA[j-1] = A_prev[j-1] - k*A_prev[i-j-1];
		for(int j=0;j<P;j++)
			A_prev[j] = AA[j];
	}
}

// Function to Calculate C vector for a frame of 320 samples
void calculate_C(int frame){
	double sigma = R[0], sum=0;
	C[frame][0] = log(sigma * sigma);
	for(int m=1;m<=P;m++){
		sum = 0;
		for(int k=1;k<=m-1;k++){
			sum += k*C[frame][k]*AA[m-k-1]/m;
		}
		C[frame][m] = AA[m-1] + sum;
	}
}

// This is a function to skip meta data which might be present in the beginning of recordings obtained via Cool Edit.
void skip_metadata(FILE *fptr){
	fseek(fptr, 0, SEEK_SET);
	int i=0;
	char data[200]; 
	int flag = 0;
	while(i++<10 &&(fscanf(fptr,"%s", data)) != EOF){
		if(flag == 0 && ((data[0] != '-') && (data[0] < '0' || data[0] > '9')))
			flag = 1;
	}
	// If there is no metadata, seek to the beginning of the file
	if(flag == 0)
		fseek(fptr, 0, SEEK_SET);
}

void copy_to_test_data(int f){
	for(int i=0;i<SAMPLES_IN_FRAME;i++)
		test_data[i] = frame_data[f*SAMPLES_IN_FRAME+i];
}

// This is a function to extract stable frames from a recording which will be used for prediction. This also normalises the samples.
int extract_stable_frame_data(int digitNumber, int utterance, int choice, int model){
	for(int i=0;i<SAMPLES_IN_FRAME*FRAMES;i++)
		frame_data[i] = 0;
	
	FILE *fptr;
	char filename[300];
	filename[0] = '\0';
	if(choice == TRAINING && model == DEFAULT_MODEL)
		sprintf(filename,"recordings/%d/obs_%d.txt",digitNumber,utterance+1);
	else if(choice == TRAINING && model == CUSTOM_MODEL)
		sprintf(filename,"custom_model/recordings/%d/obs_%d.txt",digitNumber,utterance+1);
	else 
		sprintf(filename,"testing/%d/obs_%d.txt",digitNumber,utterance+1);
	printf("%s\n",filename);
	if ((fptr = fopen(filename,"r")) == NULL){
		printf("Error! opening file");
	}
	skip_metadata(fptr);
	//normalise
	double max=0,min=0,abs_max;
	double data;
	int count=0,frames=0;
	double sum=0,energy=0;
	while((fscanf(fptr,"%lf", &data)) != EOF){
		if(data>max)
			max = data;
		if(data<min)
			min = data;
		sum += data*data;
		count++;
		if(count == 320){
			frames++;
			energy += sum/320;
			sum = 0;
			count = 0;
		}
	}
	energy = energy/frames;

	if(-1*min > max)
		abs_max = -1*min;
	else
		abs_max = max;

	//go to begining
	skip_metadata(fptr);sum = 0;
	int no_of_frames = 0;
	int index = -1;
	int flag = 0;
	while((fscanf(fptr,"%lf", &data)) != EOF){
		if(flag == 0){
			sum += data*data;
			count++;
			if(count == 320){
				sum = sum/320;
				if(sum > STABLE_FRAME_FACTOR*energy){
					flag = 1;
					index = 0;
				}
				sum = 0;
				count = 0;
			}
		}
		else if(flag == 1){
			sum += data*data;
			count++;
			if(count == 320){
				sum = sum/320;
				no_of_frames++;
				if(sum < STABLE_FRAME_FACTOR*energy){
					flag = 2;
				}
				sum = 0;
				count = 0;
			}
			frame_data[index++] = (data/max)*5000;
		}
		if(flag == 2 || index == SAMPLES_IN_FRAME*FRAMES)
			break;
	}
	fclose(fptr);
	if ((fptr = fopen(OUTPUT_PATH,"w")) == NULL){
		printf("Error! opening file");
	}
	for(int i=0;i<SAMPLES_IN_FRAME*no_of_frames;i++){
		fprintf(fptr,"%lf\n",frame_data[i]);
	}
	fclose(fptr);
	printf("No of frames = %d\n\n", no_of_frames);
	return no_of_frames;
}

// Function to compute C vectors of the frames of all recordings.
void populate_C(int no_of_frames){
	FILE *fptr;
	if ((fptr = fopen(OUTPUT_C,"w")) == NULL){
		printf("Error! opening file");
	}
	double sum;
	for(int j=0;j<no_of_frames;j++){
		copy_to_test_data(j);
		calculate_R();
		calculate_A();
		calculate_C(j);
		// Raised Sine window to C vector
		for(int l = 0;l<P+1;l++){
			if(l>=1){
				C[j][l] *= (1 + P*sin(3.14*l/P)/2);
				fprintf(fptr, "%lf\t", C[j][l]);
			}
		}
		fprintf(fptr, "\n");
	}
	fclose(fptr);
}

int read_codebook(){
	FILE *fptr;
	char line[1024];
	char *ptr;
	int count = 0;

	// Open provided_codebook.txt file 
	if ((fptr = fopen("provided_codebook.txt","r")) == NULL){
		printf("Error! opening file");
		return(1);
	}

	count = 0;

	// Read the Universe.csv file line by line
	while (fgets(line, 1024, fptr)){
		char *token;
 
		// get the first token
		token = strtok(line, "\t");
   
		// walk through other tokens
		for(int i=0;i<12;i++){
			codebook[count][i] = strtod(token, &ptr);
			token = strtok(NULL, "\t");
		}
		count++;
	}
	printf("count = %d\n",count);
	fclose(fptr);
	printf("codebook-----------------------\n");
	for(int i=0;i<32;i++){
		for(int j = 0;j<12;j++)
			printf("%g ", codebook[i][j]);
		printf("\n");
	}
	printf("\n");
	return 0;
}

// Function to compute Tokhura's Distance
void produce_observation_sequence(int no_of_frames){
	//calculating Tokhura's Distance
	double sum = 0, TD[32];
	//for each frame
	for(int i=0;i<no_of_frames;i++){
		// for each codebook entry
		for(int j=0;j<32;j++){
			TD[j] = 0;
			for(int k = 1;k<P+1;k++){
				TD[j] += w[k] * (C[i][k]-codebook[j][k])*(C[i][k]-codebook[j][k]);
			}
		}
		int index = 0;
		for(int j=0;j<32;j++){
			if(TD[j] < TD[index]){
				index = j;
			}
		}
		OBS_SEQ_GEN[i] = index+1;
	}
}

