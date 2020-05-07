// Properties.h
// CSCE 463-500
// Luke Grammer
// 9/24/19

#pragma once

#include "pch.h"

using namespace std;

// Properties struct for shared thread information (statistics, url queue, and seen hosts/ips)
struct Properties
{
	CRITICAL_SECTION critical_section;
	HANDLE eventQuit = CreateEvent(NULL, true, false, NULL);
	LONG num_threads = 0;
	LONG dns_lookups = 0;
	LONG num_downloaded_robots = 0;
	LONG num_passed_robots = 0;
	LONG num_crawled_urls_by_code[5] = { 0, 0, 0, 0, 0 };
	LONG amount_downloaded = 0;
	LONG pages_visited = 0;
	LONG num_extracted_urls = 0;
	LONG domain_links_internal = 0;
	LONG domain_links_external = 0;
	LONGLONG total_links = 0;
	vector<const char*> url_vec;
	unordered_set<DWORD> seen_ips;
	unordered_set<string> seen_hosts;
};