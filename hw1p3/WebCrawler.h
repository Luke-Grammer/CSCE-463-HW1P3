// WebCrawler.h
// CSCE 463-500
// Luke Grammer
// 9/24/19

#pragma once

class WebCrawler
{
	ParsedURL url;
	SOCKET sock;
	struct sockaddr_in server; // structure for connecting to server

	// Beginning and end time points for timer implementation
	std::chrono::time_point<std::chrono::high_resolution_clock> start_time, stop_time; 

public:
	// basic constructor initializes winsock and opens a TCP socket
	WebCrawler(ParsedURL _url = ParsedURL()); 

	// destructor cleans up winsock and closes socket
	~WebCrawler(); 

	// basic setter for the url data member
	void SetUrl(ParsedURL _url);

	// resolves DNS for the host specified by the url member and returns 0 and the IP on success or a positive error code for failure
	int ResolveDNS(DWORD &IP, bool print);

	// creates a TCP connection to a server, returns positive error code for failure and 0 for success
	int CreateConnection(bool print); 

	// writes a properly formatted HTTP query to the connected server, returns positive error code for failure and 0 for success
	int Write(std::string request_type, bool chunked, std::string request = ""); 

	// checks HTTP header in buf and returns true if the response code is between min_response and max_response (inclusive), false otherwise
	bool VerifyHeader(char *buf, int& response, int min_response, int max_response);

	// receives HTTP response from connected server
	int Read(Properties* p, char* &buf, int read_limit, int &cur_size, int &allocated_size, bool print); 

	// dechunks a chunked server response in place. returns a positive error code in case of failure or 0 for success 
	int Dechunk(char* buf, int& cur_size, int& allocated_size);

	// parses HTTP response and finds number of links in HTML buffer
	int Parse(Properties* p, char* buf, int size, bool print); 

	// resets socket connection
	int ResetConnection();

	/*
	 * Connects to a URL and issues an HTTP 1.1 request. If the request is chunked, 
	 * dechunk and print out HTTP header. If request is not chunked, 
	 * skip the dechunking step and print the HTTP response header.
	 * Returns a negative number in case of failure or 0 to indicate success.
	 */
	static int VisitSingleURL(char* url);

	/*
	 * Crawls through a list of URLs in a given input queue, initiating connection
     * if the crawler has not attempted to connect to the host before and
     * first requesting the HTTP header for /robots.txt. If robots.txt is not found
     * (4XX) response code then the page specified by the URL is requested and
     * parsed to find the number of links on the page.
	*/
	static void CrawlUrls(LPVOID properties);
};

