// ParsedURL.h
// CSCE 463-500
// Luke Grammer
// 9/24/19

#pragma once

// basic struct for containing a URL
struct ParsedURL
{   
	// status will be 0 if a URL has been parsed successfully, negative if it has not been parsed, and positive if parsed with an error
	int status; 
	int port;
	std::string scheme, host, query, path, request;

	// basic default constructor
	ParsedURL() : scheme{ "" }, host{ "" }, port{ Constants::DEFAULT_PORT }, query{ "" }, path{ "/" }, request{ "" }, status{ -1 } {} 

	/* 
	 * input:
	 *   - url: a string in URL format (<scheme>://<host>[:<port>][/<path>][?<query>][#<fragment>])
	 * 
	 * return: a ParsedURL object with data members initialized to the parsed contents of url, 
	 *         if the URL could not be successfully parsed, returned object's 'valid' member will
	 *         be false.
	 */
	static ParsedURL ParseUrl(std::string url); 
};