// StatsManager.cpp
// CSCE 463-500
// Luke Grammer
// 9/24/19

#include "pch.h"

using namespace std;

// print running statistics for webcrawling worker threads at a fixed 2s interval
void StatsManager::PrintStats(LPVOID properties)
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

	int total_urls_crawled = 0;
	size_t total_downloaded = 0;
	Properties* p = (Properties*)properties;
	std::chrono::time_point<std::chrono::high_resolution_clock> initial_start_time = std::chrono::high_resolution_clock::now();
	std::chrono::time_point<std::chrono::high_resolution_clock> segment_start_time = std::chrono::high_resolution_clock::now();
	std::chrono::time_point<std::chrono::high_resolution_clock> stop_time;

	// until shutdown request has been sent by main
	while (WaitForSingleObject(p->eventQuit, Constants::STATS_INTERVAL * 1000) == WAIT_TIMEOUT)
	{
		// enter critical section
		EnterCriticalSection(&p->critical_section);
		stop_time = std::chrono::high_resolution_clock::now();

		total_urls_crawled = 0;
		for (int i = 0; i < 4; i++)
			total_urls_crawled += p->num_crawled_urls_by_code[i];

		// print statistics
		std::printf("[%3d] %6d Q %6d E %7d H %6d D %6d I %5d R %5d C %5d L %4dK\n",
			(int)std::chrono::duration_cast<std::chrono::seconds>(stop_time - initial_start_time).count(),
			p->num_threads, (int)p->url_vec.size(), p->num_extracted_urls, (int)p->seen_hosts.size(),
			p->dns_lookups, (int)p->seen_ips.size(), p->num_passed_robots, total_urls_crawled,
			(int) (p->total_links / 1000));

		// time since last print
		size_t segment_time = std::chrono::duration_cast<std::chrono::milliseconds>(stop_time - segment_start_time).count();

		// print extra stats
		std::printf("\t*** crawling %.1f pps @ %.1f Mbps\n", (p->pages_visited * 1000.0) / segment_time, ((UINT64)p->amount_downloaded * 8 * 1000.0) / (1000000.0 * segment_time));
		
		// keep track of total downloaded
		total_downloaded += p->amount_downloaded;

		// reset number of pages visited
		p->pages_visited = 0;
		p->amount_downloaded = 0;
		
		// exit the critical section
		LeaveCriticalSection(&p->critical_section);
		segment_start_time = std::chrono::high_resolution_clock::now();
	}

	total_urls_crawled = 0;
	for (int i = 0; i < 5; i++)
		total_urls_crawled += p->num_crawled_urls_by_code[i];
	stop_time = std::chrono::high_resolution_clock::now();
	double runtime = std::chrono::duration_cast<std::chrono::milliseconds>(stop_time - initial_start_time).count() / 1000.0;
	
	if (runtime + .001 > 0 && runtime - .001 < 0)
	{
		std::printf("calculated runtime is 0\n");
		return;
	}

	// print final statistics
	std::printf("\n");
	std::printf("Extracted %d URLs @ %d/s\n", p->num_extracted_urls, p->num_extracted_urls / (int)runtime);
	std::printf("Looked up %d DNS names @ %d/s\n", p->dns_lookups, p->dns_lookups / (int)runtime);
	std::printf("Downloaded %d robots @ %d/s\n", p->num_downloaded_robots, p->num_downloaded_robots / (int)runtime);
	std::printf("Crawled %d pages @ %d/s (%.2f MB)\n", total_urls_crawled, total_urls_crawled / (int)runtime, total_downloaded / 1000000.0);
	std::printf("Parsed %zu links @ %zu/s\n", p->total_links, p->total_links / (int)runtime);
	std::printf("HTTP codes: 2xx = %d, 3xx = %d, 4xx = %d, 5xx = %d, other = %d\n", p->num_crawled_urls_by_code[0], p->num_crawled_urls_by_code[1],
		p->num_crawled_urls_by_code[2], p->num_crawled_urls_by_code[3],
		p->num_crawled_urls_by_code[4]);
	std::printf("\nParsed %d links in the %s domain\n", p->domain_links_internal + p->domain_links_external, Constants::HOST_TO_CHECK.c_str());
	std::printf("\tInternal: %d\n", p->domain_links_internal);
	std::printf("\tExternal: %d\n", p->domain_links_external);
}