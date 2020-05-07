// hw1p3.cpp
// CSCE 463-500
// Luke Grammer
// 9/24/19

#include "pch.h"

#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h> // libraries to check for memory leaks

#pragma comment(lib, "ws2_32.lib")

using namespace std;

/*
 * Function: main
 * ------------------
 * Simple driver function to validate command line arguments, read URL data into a file and create worker threads and a stats thread 
 * expects two command line arguments: number of threads and input file
 *
 * input:
 *   - argc: count of command line arguments
 *   - argv: array of strings with three elements ["hw1p3.exe", "<number of threads>", "<input file>"]
 *
 * return: an status code that will be 1 in the case that an error is encountered,
 *         or 0 for successful execution
 */
int main(int argc, char** argv)
{
	// debug flag to check for memory leaks
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); 
	
	int num_threads = 0;
	Properties properties;
	
	// make sure command line arguments are valid
	if (argc != 2 && argc != 3)
	{  
		(argc < 2) ? printf("too few arguments") : printf("too many arguments");
		printf("\nusage: hw1p3.exe <1-5000> <filename>\n");
		printf("or hw1p3.exe http://host[:port][/path][?query][#fragment]\n");
		return(EXIT_FAILURE);
	}
	
	// extra credit functionality
	if (argc == 2)
	{
		if (WebCrawler::VisitSingleURL(argv[1]) == 0)
			return(EXIT_SUCCESS);
		else
			return(EXIT_FAILURE);
	}

	// convert the number of threads to an int and validate its range
	try
	{
		num_threads = stoi(argv[1]);
		if (num_threads < Constants::MIN_NUM_THREADS || num_threads > Constants::MAX_NUM_THREADS)
			throw out_of_range("");
	}
	catch (...)
	{ 
		printf("supplied argument for thread count invalid: %s\n", argv[1]);
		return(EXIT_FAILURE);
	}

	FileHandler* handler = new FileHandler(argv[2]);

	// get size of input file
	long long file_size = handler->GetFileSize();
	if (file_size < 0)
		return(EXIT_FAILURE);

	// read file into url queue
	if (handler->ReadFile(properties.url_vec) < 0)
		return(EXIT_FAILURE);

	printf("Opened %s with size %lld \n\n", argv[2], file_size);
	delete handler;

	HANDLE stats_handle;
	HANDLE* worker_handles = new HANDLE[num_threads];

	// Initialize critical section
	InitializeCriticalSection(&properties.critical_section);
	properties.num_threads = num_threads;
	
	// Create Stats thread
	stats_handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)StatsManager::PrintStats, &properties, 0, NULL);
	if (stats_handle == NULL)
	{
		printf("Could not create stats thread! exiting...\n");
		return(EXIT_FAILURE);
	}
	
	// Create worker thread(s)
	for (int i = 0; i < num_threads; i++)
	{
		worker_handles[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WebCrawler::CrawlUrls, &properties, 0, NULL);
		if (worker_handles[i] == NULL)
		{
			printf("Could not create worker thread! exiting...\n");
			return(EXIT_FAILURE);
		}
	}

	// Wait for num_threads worker threads to finish
	for (int i = 0; i < num_threads; i++)
	{
		WaitForSingleObject(worker_handles[i], INFINITE);
		CloseHandle(worker_handles[i]);
		InterlockedDecrement((LONG volatile*) &properties.num_threads);
	}
	
	// Signal stats thread to quit
	if (properties.eventQuit != NULL)
		SetEvent(properties.eventQuit);

	WaitForSingleObject(stats_handle, INFINITE);
	CloseHandle(stats_handle);

	DeleteCriticalSection(&properties.critical_section);
	delete[] worker_handles;
	return(EXIT_SUCCESS);
}
