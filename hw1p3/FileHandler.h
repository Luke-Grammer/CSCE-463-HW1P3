// FileHandler.h
// CSCE 463-500
// Luke Grammer
// 9/24/19

#pragma once

// class for opening, reading, and getting size of URL file
class FileHandler 
{
	// used to store handle for open file
	FILE* file = NULL;

	// used to store the name of the file
	const char* file_name;

public:
	// basic constructor opens file specified by name for reading in binary mode
	FileHandler(const char* name);

	// basic destructor closes file if open
	~FileHandler();

	// returns the size of the open file or -1 if file is not opened or empty
	// also resets position of file pointer to beginning of file
	long long GetFileSize();

	// reads the file into a queue structure of urls 
	int ReadFile(std::vector<const char*> &url_vec);
};
