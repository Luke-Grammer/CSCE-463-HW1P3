// WebCrawler.cpp
// CSCE 463-500
// Luke Grammer
// 9/24/19

#include "pch.h"

using namespace std;

// basic constructor initializes winsock and opens a TCP socket
WebCrawler::WebCrawler(ParsedURL _url) : server{ NULL }
{   
	WSADATA wsa_data;
	WORD w_ver_requested;
	
	url = _url;

	//initialize WinSock
	w_ver_requested = MAKEWORD(2, 2);
	if (WSAStartup(w_ver_requested, &wsa_data) != 0) {
		printf("\tWSAStartup error %d\n", WSAGetLastError());
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	// open a TCP socket
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		printf("\tsocket() generated error %d\n", WSAGetLastError());
		WSACleanup();
		exit(EXIT_FAILURE);
	}
}

// destructor cleans up winsock and closes socket
WebCrawler::~WebCrawler() 
{   
	closesocket(sock);
	WSACleanup();
}

// basic setter for the url data member
void WebCrawler::SetUrl(ParsedURL _url)
{
	url = _url;
}

// resolves DNS for the host specified by the url member and returns an IP address or a positive error code for failure
int WebCrawler::ResolveDNS(DWORD &IP, bool print)
{
	char* host;
	struct in_addr addr;
	struct hostent* remote;

	// URL not successfully parsed yet 
	if (url.status != 0)
		return 1; 
	
	host = (char*) malloc(MAX_URL_LEN);
	
	// failure allocating memory for hostname
	if (host == NULL)
		return 2;

	// need to put the hostname in a C string
	strcpy_s(host, MAX_URL_LEN, url.host.c_str());

	if (print)
		start_time = chrono::high_resolution_clock::now();
    
	// first assume that the hostname is an IP address
	IP = inet_addr(host); 
	
	// host is not a valid IP, do a DNS lookup
	if (IP == INADDR_NONE)
	{   
		if ((remote = gethostbyname(host)) == NULL) 
		{
			// failure in gethostbyname
			free(host);
			return WSAGetLastError();
		}
		// take the first IP address and copy into sin_addr
		else 
		{
			IP = *(u_long*)remote->h_addr;
			addr.s_addr = IP;
			memcpy((char*) &(server.sin_addr), remote->h_addr, remote->h_length);
			if (print)
			{
				stop_time = chrono::high_resolution_clock::now();
				printf("done in %" PRIu64 " ms, found %s\n",
					chrono::duration_cast<chrono::milliseconds>
					(stop_time - start_time).count(), inet_ntoa(addr));
			}
		}
	}
	// host is a valid IP, directly drop its binary version into sin_addr, stop timer and print
	else
	{
		if (print)
		{
			stop_time = chrono::high_resolution_clock::now();
			printf("done in %" PRIu64 " ms, found %s\n",
				   chrono::duration_cast<chrono::milliseconds>
				   (stop_time - start_time).count(), host);
		}
		server.sin_addr.S_un.S_addr = IP;
	}

	free(host);
	return 0;
}

// creates a TCP connection to a server, returns a postitive error code for failure and 0 for success
int WebCrawler::CreateConnection(bool print)
{ 
	// URL not successfully parsed yet 
	if (url.status != 0)
		return 1; 

	// set up the port # and protocol type
	server.sin_family = AF_INET;
	server.sin_port = htons(url.port); 

	if (print)
		start_time = chrono::high_resolution_clock::now();
	
	// connect to the server
	if (connect(sock, (struct sockaddr*) &server, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
		return WSAGetLastError(); // failure on connect

	if (print)
	{
		stop_time = chrono::high_resolution_clock::now();
		printf("done in %" PRIu64 " ms\n",
			   chrono::duration_cast<chrono::milliseconds>
			   (stop_time - start_time).count());
	}

	return 0;
}

// writes a properly formatted HTTP query to the connected server, returns a positive error code for failure and 0 for success
int WebCrawler::Write(string request_type, bool chunked, string request)
{
	string version = "1.0";

	if (chunked)
		version = "1.1";

	if (request == "")
		request = url.request;

	string http_request = request_type + " " + request + " HTTP/" + version + "\r\nUser-agent: " + 
		        Constants::AGENT_NAME + "\r\nHost: " + url.host + "\r\nConnection: close\r\n\r\n";

	if (send(sock, http_request.c_str(), (int) strlen(http_request.c_str()), NULL) < 0)
		return WSAGetLastError(); // send failure

	return 0;
}

// receives HTTP response from connected server
int WebCrawler::Read(Properties* p, char* &buf, const int read_limit, int &cur_size, int &allocated_size, bool print)
{
	// set socket timeout value
	struct timeval timeout; 
	timeout.tv_sec = Constants::TIMEOUT_SECONDS; 
	timeout.tv_usec = 0;

	int ret = 0;
	fd_set fd;
	FD_ZERO(&fd);

	cur_size = 0;

	// start connection timer
	start_time = chrono::high_resolution_clock::now();
	stop_time = chrono::high_resolution_clock::now();
	while ((chrono::duration_cast<chrono::milliseconds>(stop_time - start_time).count() / 1000.0) < Constants::MAX_CONNECTION_TIME &&
		   cur_size < read_limit)
	{
		FD_SET(sock, &fd);

		// wait to see if socket has any data
		ret = select((int) sock + 1, &fd, NULL, NULL, &timeout);
		if (ret > 0)
		{
			// new data available
			int bytes = recv(sock, buf + cur_size, (int) (allocated_size - cur_size), NULL);
			if (bytes < 0)
				return WSAGetLastError(); // recv failure

			InterlockedAdd((volatile LONG*) &p->amount_downloaded, bytes);

			// advance current position by number of bytes read
			cur_size += bytes; 
			
			// buffer needs to be expanded
			if (allocated_size - cur_size < Constants::BUF_SIZE_THRESHOLD)
			{   
				// make sure allocated_size will not overflow
				if (2 * allocated_size < allocated_size) 
				{   
					free(buf);
					buf = NULL;
					cur_size = 0;
					allocated_size = 0;
					return 1; // buffer overflow
				}

			    // expand memory for buffer, making sure the expansion succeeds
				char* temp = (char*) realloc(buf, 2 * (__int64) allocated_size); 
				if (temp == NULL)
				{
					free(buf);
					buf = NULL;
					cur_size = 0;
					allocated_size = 0;
					return 2; // buffer reallocation failure
				}

                // double allocated size with each expansion (higher overhead but faster)
				allocated_size *= 2; 
				buf = temp;
			}
			// connection closed
			if (bytes == 0) 
			{
				if (buf == NULL)
				{
					cur_size = 0;
					allocated_size = 0;
					return 3; // buffer null
				}

				stop_time = chrono::high_resolution_clock::now();
				/* Null-terminate buffer
				 *
				 * Warning C6386 due to indexing by cur_size, but buffer overflow is not possible because 
				 * allocated_size is strictly > cur_size while BUF_SIZE_THRESHOLD > 0 
				 */
				buf[cur_size] = '\0'; 

				if (print)
				{
					printf("done in %" PRIu64 " ms with %d bytes\n",
						chrono::duration_cast<chrono::milliseconds>
						(stop_time - start_time).count(), cur_size);
				}
				return 0;
			}
		}
		// socket timed out
		else if (ret == 0) 
		{   
			stop_time = chrono::high_resolution_clock::now();
			return 5; // socket timeout
		}
		else
			return WSAGetLastError(); // select failure

		stop_time = chrono::high_resolution_clock::now();
	}
	// buf has advanced more than read_limit bytes
	if (cur_size >= read_limit)
		return 6; // exceeded max download 

	return 7; // connection timeout
}

// dechunks a chunked server response in place. returns a positive error code in case of failure or 0 for success 
int WebCrawler::Dechunk(char* buf, int& cur_size, int& allocated_size)
{
	// make sure response is chunked
	char* chunked_response = StrStrIA(buf, "Transfer-Encoding: chunked\r\n");
	if (chunked_response == NULL)
	{
		printf("response is not chunked\n");
		return 1;
	}

	// ignore header and keep track of beginning of payload information
	char* payload_start = StrStrIA(buf, "\r\n\r\n");
	if (payload_start == NULL)
	{
		printf("unexpected error stripping HTTP header\n");
		return 1;
	}
	// advance to beginning of payload information if header was stripped successfully
	payload_start += 4;

	char* payload = payload_start;
	int size = 0;
	int consumed = 0;
	int amount_advanced = 0;
	// while chunks can still be extracted
	while (sscanf_s(payload, "%x%n", &size, &consumed) > 0)
	{
		// advance payload past chunk header
		payload += (__int64) consumed + 2;
		// shift bytes back to remove chunk headers
		memcpy(payload_start + amount_advanced, payload, size);
		amount_advanced += size;
		// advance payload to beginning of next chunk
		payload += size;
	}

	// check if message was fully de-chunked
	if (size != 0)
	{
		printf("payload data corrupted, dechunking was unsuccessful\n");
		printf("was the server response properly formatted?\n");
		return 1;
	}

	// print info and return
	printf("body size was %zu, ", strlen(payload_start));
	payload_start[amount_advanced] = '\0';
	printf("now %zu\n", strlen(payload_start));

	cur_size = (int) strlen(buf);
	return 0;
}

// checks HTTP header in buf and returns true if the response code is between min_response and max_response (inclusive), false otherwise
bool WebCrawler::VerifyHeader(char* buf, int &response, int min_response, int max_response)
{
	response = -1;
	// get response code from HTTP header
	char* response_pos = StrStrIA(buf, "HTTP/");

	// response is found
	if (response_pos != NULL)
	{   
		// response code could not be extracted
		if (sscanf_s(response_pos, "%*s %d", &response) <= 0)
			return false;
	}
	else
		return false; // Non-HTTP Header

	if (response >= min_response && response <= max_response)
		return true;

	return false;
}

// parses HTTP response and finds number of links in HTML buffer
int WebCrawler::Parse(Properties* p, char *buf, int size, bool print)
{
	ParsedURL link_url;
	HTMLParserBase parser;
	int num_links = -1;
	char *base_url = NULL;   
	string base_url_string = url.scheme + "://" + url.host;

	base_url = (char*)malloc(MAX_URL_LEN); 
	if (base_url == NULL)
		return -1; // malloc failure for base URL

	// create C-style string with base URL
	strcpy_s(base_url, MAX_URL_LEN, base_url_string.c_str());

	if (print)
		start_time = chrono::high_resolution_clock::now();

	// get number of links from response
	char* link_buffer = parser.Parse(buf, (int)size, base_url, (int)strlen(base_url), &num_links);
	if (num_links < 0)
	{
		free(base_url);
		return -2; // HTML Parsing error
	}

	// Extra functionality to detect internal and external links from within a specified domain
	for (int i = 0; i < num_links; i++)
	{
		string link(link_buffer);

		link_url = ParsedURL::ParseUrl(link);

		if (link_url.status == 0 && Constants::HOST_TO_CHECK.size() <= link_url.host.size())
		{
			if (equal(Constants::HOST_TO_CHECK.rbegin(), Constants::HOST_TO_CHECK.rend(), link_url.host.rbegin()))
			{	
				if (Constants::HOST_TO_CHECK.size() <= url.host.size())
				{
					if (equal(Constants::HOST_TO_CHECK.rbegin(), Constants::HOST_TO_CHECK.rend(), url.host.rbegin()))
						InterlockedIncrement((volatile LONG*)& p->domain_links_internal);
					else
						InterlockedIncrement((volatile LONG*)& p->domain_links_external);
				}
				else
					InterlockedIncrement((volatile LONG*)& p->domain_links_external);
			}
		}

		link_buffer += link.length() + 1;
	}


	if (print)
	{
		stop_time = chrono::high_resolution_clock::now();
		printf("done in %" PRIu64 " ms with %d links\n", chrono::duration_cast<chrono::milliseconds>(stop_time - start_time).count(), num_links);
		printf("___________________________________________________________________________________\n");

		// extract and print HTTP header
		char* header_end = strstr(buf, "\r\n\r\n");
		if (header_end == NULL)
		{
			printf("unexpected error printing HTTP header\n");
			free(base_url);
			return -1;
		}

		*header_end = '\0';
		printf("%s\r\n", buf);
	}

	// clean up
	free(base_url); 
	return num_links;
}

// resets socket connection
int WebCrawler::ResetConnection()
{
	if (closesocket(sock) == SOCKET_ERROR)
		return 1;

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
		return 2;

	return 0;
}

/*
 * Connects to a URL and issues an HTTP 1.1 request. If the request is chunked, dechunk and print out HTTP header.
 * If request is not chunked, skip the dechunking step and print the HTTP response header.
 * Returns a negative number in case of failure or 0 to indicate success.
 */
int WebCrawler::VisitSingleURL(char* url)
{
	Properties p;
	ParsedURL parsed_url;
	WebCrawler crawler;
	string url_string(url);
	int return_val = 0;

	// buffer should be allocated before it is used
	char* buffer = (char*)malloc(Constants::INITIAL_BUF_SIZE);
	if (buffer == NULL)
	{
		printf("malloc failed for buffer");
		return -1;
	}
	int cur_buf_size = 0;
	int allocated_size = Constants::INITIAL_BUF_SIZE;

	printf("URL: %s\n", url_string.c_str());
	printf("\tParsing URL... ");
	parsed_url = ParsedURL::ParseUrl(url_string);

	// check if URL was parsed correctly
	if (parsed_url.status != 0)
	{
		printf("ParseUrl returned error code %d", parsed_url.status);
		if (buffer != NULL)
			free(buffer);
		return -1;
	}

	printf("host %s, port %d, request %s\n", parsed_url.host.c_str(), parsed_url.port, parsed_url.request.c_str());

	// link the parsed URL with the crawler
	crawler.SetUrl(parsed_url);

	// do DNS
	printf("\tDoing DNS... ");
	DWORD IP;
	return_val = crawler.ResolveDNS(IP, true);
	if (return_val != 0)
	{
		printf("ResolveDNS returned error code %d\n", return_val);
		if (buffer != NULL)
			free(buffer);
		return -1;
	}

	// connect to page
	printf("      * Connecting to page... ");
	return_val = crawler.CreateConnection(true);
	if (return_val != 0)
	{
		printf("CreateConnection returned error code %d\n", return_val);
		if (buffer != NULL)
			free(buffer);
		return -1;
	}

	// send request
	return_val = crawler.Write("GET", true);
	if (return_val != 0)
	{
		printf("Write returned error code %d\n", return_val);
		if (buffer != NULL)
			free(buffer);
		return -1;
	}

	// read response
	printf("\tLoading... ");
	return_val = crawler.Read(&p, buffer, Constants::MAX_PAGE_SIZE, cur_buf_size, allocated_size, true);
	if (return_val != 0)
	{
		printf("Read returned error code %d\n", return_val);
		if (buffer != NULL)
			free(buffer);
		return -1;
	}

	// check if response code is valid
	printf("\tVerifying header... ");
	if (!crawler.VerifyHeader(buffer, return_val, 200, 299))
	{
		if (return_val < 0)
			printf("server provided non-http response\n");
		else
			printf("status code %d\n", return_val);

		if (buffer != NULL)
			free(buffer);
		return -1;
	}
	printf("status code %d\n", return_val);

	// check if response is chunked and de-chunk if necessary
	char* chunked_response = StrStrIA(buffer, "Transfer-Encoding: chunked\r\n");
	if (chunked_response != NULL)
	{
		printf("\tDechunking... ");
		if (crawler.Dechunk(buffer, cur_buf_size, allocated_size) != 0)
		{
			if (buffer != NULL)
				free(buffer);
			return -1;
		}
	}

	// parse links in page
	printf("      + Parsing page... ");
	return_val = crawler.Parse(&p, buffer, cur_buf_size, true);
	if (return_val < 0)
	{
		printf("Parse returned error code %d\n", return_val);
		if (buffer != NULL)
			free(buffer);
		return -1;
	}

	if (buffer != NULL)
		free(buffer);

	return(EXIT_SUCCESS);
}

/*
 * Function: CrawlUrls
 * ------------------
 * Crawls through a list of URLs in a given input file, initiating connection
 * if the crawler has not attempted to connect to the host before and
 * first requesting the HTTP header for /robots.txt. If robots.txt is not found
 * (4XX) response code then the page specified by the URL is requested and
 * parsed to find the number of links on the page.
 *
 * input:
 *   - properties: a struct shared among all worker threads that contains several shared 
 *                 resources for keeping track of stats and seen URLs and IPs 
 */
void WebCrawler::CrawlUrls(LPVOID properties)
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);
	Properties* p = (Properties*)properties;
	WebCrawler crawler;
	int prev_size = 0;
	int cur_buf_size = 0;
	int allocated_size = 0;
	char* buffer = NULL;
	int return_code = -1;
	int http_response = -1;

	while (true)
	{
		// Critical section popping new URL from shared queue
		EnterCriticalSection(&p->critical_section);
		if (p->url_vec.size() == 0)
		{
			LeaveCriticalSection(&p->critical_section);
			break;
		}
		
		const char* temp_url = p->url_vec.back();
		string url(temp_url);
		free((void*)temp_url);
		p->url_vec.pop_back();

		LeaveCriticalSection(&p->critical_section);

		InterlockedIncrement((volatile LONG*) &p->num_extracted_urls);

		return_code = crawler.ResetConnection();
		if (return_code != 0)
		{
			printf("thread %u failed to reset socket connection with error %d, terminating thread\n", GetCurrentThreadId(), WSAGetLastError());
			break;
		}

		// buffer should be de-allocated if it is too large
		if (allocated_size > Constants::BUF_RESET_THRESHOLD)
		{
			free(buffer);
			buffer = NULL;
			cur_buf_size = 0;
			allocated_size = 0;
		}

		// buffer should be allocated before it is used
		if (buffer == NULL)
		{
			buffer = (char*)malloc(Constants::INITIAL_BUF_SIZE);
			if (buffer == NULL)
			{
				printf("thread %u failed to allocate space for buffer, terminating thread\n", GetCurrentThreadId());
				break;
			}
			allocated_size = Constants::INITIAL_BUF_SIZE;
			cur_buf_size = 0;
		}
		 
		// check if there was an error parsing URL and print error message if parse was not successful

		crawler.SetUrl(ParsedURL::ParseUrl(url));
		if (crawler.url.status != 0)
			continue;

		// check uniqueness of hostname and IP address
		// --------------------------------------------------------------------
		// check host uniqueness
		EnterCriticalSection(&p->critical_section);
		prev_size = (int) p->seen_hosts.size();
		p->seen_hosts.insert(crawler.url.host);
		if (p->seen_hosts.size() <= prev_size)
		{
			// test failed
			LeaveCriticalSection(&p->critical_section);
			continue;
		}
		// test succeeded (new unique host added)
		LeaveCriticalSection(&p->critical_section);

		// do DNS lookup
		DWORD IP = NULL;
		return_code = crawler.ResolveDNS(IP, false);

		if (return_code != 0)
			continue;

		if (IP == NULL)
			continue;

		InterlockedIncrement((volatile LONG*) &p->dns_lookups);

		// check IP uniqueness
		EnterCriticalSection(&p->critical_section);
		prev_size = (int) p->seen_ips.size();
		p->seen_ips.insert(IP);
		if (p->seen_ips.size() <= prev_size)
		{
			// test failed
			LeaveCriticalSection(&p->critical_section);
			continue;
		}
		// test succeeded (new unique IP added)
		LeaveCriticalSection(&p->critical_section);

		// check /robots.txt
		// --------------------------------------------------------------------------
		return_code = crawler.CreateConnection(false);
		if (return_code != 0)
			continue;

		// write robots request
		return_code = crawler.Write("HEAD", false, "/robots.txt");
		if (return_code != 0)
			continue;

		// read robots response
		return_code = crawler.Read(p, buffer, Constants::MAX_ROBOTS_SIZE, cur_buf_size, allocated_size, false);
		if (return_code != 0)
			continue;

		InterlockedIncrement((volatile LONG*) &p->num_downloaded_robots);

		// check if robots is present
		if (!crawler.VerifyHeader(buffer, http_response, 400, 499))
			continue;

		InterlockedIncrement((volatile LONG*) &p->num_passed_robots);

		// connect to page
		// --------------------------------------------------------------------
		return_code = crawler.ResetConnection();
		if (return_code != 0)
		{
			printf("thread %u failed to reset socket connection with error %d, terminating thread\n", GetCurrentThreadId(), WSAGetLastError());
			break;
		}

		return_code = crawler.CreateConnection(false);
		if (return_code != 0)
		{
			//printf("failed in connect, return_code %d\n", return_code);
			continue;
		}

		// write page request
		return_code = crawler.Write("GET", false);
		if (return_code != 0)
		{
			printf("failed in write, return code %d\n", return_code);
			continue;
		}

		// read page response
		return_code = crawler.Read(p, buffer, Constants::MAX_PAGE_SIZE, cur_buf_size, allocated_size, false);
		if (return_code != 0)
		{
			//printf("failed in read, code %d\n", return_code);
			continue;
		}

		InterlockedIncrement((volatile LONG*) &p->pages_visited);

		// check header code
		if (!crawler.VerifyHeader(buffer, http_response, 200, 299))
		{
			if (http_response < 0)
			{
				InterlockedIncrement((volatile LONG*) &p->num_crawled_urls_by_code[4]);
			}
			else
			{
				if (http_response >= 300 && http_response <= 399)
					InterlockedIncrement((volatile LONG*) &p->num_crawled_urls_by_code[1]);
				else if (http_response >= 400 && http_response <= 499)
					InterlockedIncrement((volatile LONG*) &p->num_crawled_urls_by_code[2]);
				else if(http_response >= 500 && http_response <= 599)
					InterlockedIncrement((volatile LONG*) &p->num_crawled_urls_by_code[3]);
			}
			continue;
		}
		InterlockedIncrement((volatile LONG*) &p->num_crawled_urls_by_code[0]);

		// parse page
		return_code = crawler.Parse(p, buffer, cur_buf_size, false);
		if (return_code < 0)
			continue;

		InterlockedAdd64((volatile LONGLONG*) &p->total_links, return_code);
	}

	if (buffer != NULL)
		free(buffer);

	return;
}