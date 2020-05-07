// Constants.h
// CSCE 463-500
// Luke Grammer
// 9/24/19

#pragma once

namespace Constants
{
	// smallest valid number of threads
	const unsigned MIN_NUM_THREADS = 1;

	// largest valid number of threads
	const unsigned MAX_NUM_THREADS = 5000;

	// max size of robots.txt to download (16KB)
	const int   MAX_ROBOTS_SIZE = 16 * 1024;

	// max size of page to download (2MB)
	const int   MAX_PAGE_SIZE = 2 * 1024 * 1024;

	// crawler agent name
	const std::string AGENT_NAME = "CPPWebCrawler/1.3";
	
	// largest valid port number
	const unsigned MAX_PORT = 65535;

	// smallest valid port number
	const unsigned MIN_PORT = 1;

	// default port number
	const unsigned DEFAULT_PORT = 80;

	// initial buffer size is INITIAL_BUF_SIZE
	const int INITIAL_BUF_SIZE = 8 * 1024;

	// if buffer size is greater than BUF_RESET_THRESHOLD, reset buffer
	const int BUF_RESET_THRESHOLD = 32 * 1024;

	// allocate more space for buffer is it is within BUF_SIZE_THRESHOLD bytes of filling up
	const int BUF_SIZE_THRESHOLD = 1024;

	// max time to download before connection times out in seconds
	const int MAX_CONNECTION_TIME = 10;

	// max time without read response before socket times out in seconds
	const int TIMEOUT_SECONDS = 10;

	// timer interval for stats thread in seconds
	const int STATS_INTERVAL = 2;

	// check parsed links to see if they match a certain domain
	const string HOST_TO_CHECK = "tamu.edu";
}