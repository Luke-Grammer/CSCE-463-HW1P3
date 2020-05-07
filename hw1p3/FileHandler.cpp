// FileHandler.cpp
// CSCE 463-500
// Luke Grammer
// 9/24/19

#include "pch.h"

using namespace std;

 // basic constructor opens file specified by name for reading in binary mode
FileHandler::FileHandler(const char* name)
{
	file_name = name;

	// open input file for reading
	if (fopen_s(&file, file_name, "rb") != 0)
	{
		printf("%s could not be opened for reading\n", file_name);
		exit(EXIT_FAILURE);
	}
}

// basic destructor closes file if open
FileHandler::~FileHandler()
{
	// clean up by closing file
	if (file)
	{
		if (fclose(file) != 0)
		{
			printf("file could not be closed\n");
			exit(EXIT_FAILURE);
		}
	}
}

/*
 * Function: GetFileSize
 * ------------------
 * returns the size of the open file or 0 if file is not opened or empty
 * also resets position of file pointer to beginning of file
 *
 * return: the size of the file owned by FileHandler if successful, -1 otherwise
 */
long long FileHandler::GetFileSize()
{
	if (file)
	{
		size_t file_size = 0;

		if (fseek(file, NULL, SEEK_END) != 0)
		{
			printf("error: there was a problem reading the size of the file\n");
			return -1;
		}

		file_size = ftell(file);

		if (fseek(file, NULL, SEEK_SET) != 0)
		{
			printf("error: there was a problem reading the size of the file\n");
			return -1;
		}

		if (file_size == 0)
		{
			printf("error: the file is empty\n");
			return -1;
		}

		return file_size;
	}
	else
		printf("error: file not open\n");
	return -1;
}

/*
 * Function: ReadFile
 * ------------------
 * reads the file into a queue structure of urls
 *
 * input:
 *   - url_queue: an initially empty queue of urls that is populated by the file contents
 *
 * return: modifies the url queue passed into the function 
 *         and also returns 0 on success or -1 on failure
 */
int FileHandler::ReadFile(vector<const char*> &url_vec)
{
	const char* temp_string;
	char* url = (char*)malloc(MAX_URL_LEN);
	string url_string;
	if (url == NULL)
	{
		printf("error: malloc failed for url");
		return -1;
	}

	while (fgets(url, MAX_URL_LEN, file) != NULL)
	{
		// parse the URL read from the file
		url_string = url;

		// remove carriage return and newline since they are not valid URI characters
		url_string.erase(remove(url_string.begin(), url_string.end(), '\r'), url_string.end());
		url_string.erase(remove(url_string.begin(), url_string.end(), '\n'), url_string.end());

		temp_string = _strdup(url_string.c_str());
		
		url_vec.push_back(temp_string);
	}

	free(url);
	return 0;
}