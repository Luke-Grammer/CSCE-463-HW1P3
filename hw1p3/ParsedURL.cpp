// ParsedURL.cpp
// CSCE 463-500
// Luke Grammer
// 9/24/19

#include "pch.h"

using namespace std;

/* 
 * Function: ParseUrl
 * ------------------
 * Parses a given input URL string and returns a ParsedURL struct with members 
 * initialized to parsed values
 *
 * input:
 *   - url: a string in URL format (<scheme>://<host>[:<port>][/<path>][?<query>][#<fragment>])
 * 
 * return: a ParsedURL object with data members initialized to the parsed contents of url,
 *         if the URL could not be successfully parsed, returned object's 'status' member will
 *         contain a positive error code.
 */
ParsedURL ParsedURL::ParseUrl(string url)
{  
	ParsedURL return_val;

	if (url.empty())
	{
		return_val.status = 1; // empty URL code
		return return_val;
	}

	size_t scheme_loc = url.find("://");
	if (scheme_loc != string::npos)
	{
		return_val.scheme = url.substr(0, scheme_loc);

		// HTTP is the only currently supported scheme
		if (return_val.scheme != "http")
		{ 
			return_val.status = 2; // invalid scheme code
			return return_val;
		}

		url = url.substr(scheme_loc + 3, url.length() - (scheme_loc + 3));
	}
	// no scheme found
	else
	{ 
		return_val.status = 3; // missing scheme code
		return return_val;
	}

	size_t fragment_loc = url.find("#");
	if (fragment_loc != string::npos)
	{ 
		// fragment not needed for request, can be stripped
		url = url.substr(0, fragment_loc);
	}

	size_t query_loc = url.find("?");
	if (query_loc != string::npos)
	{
		return_val.query = url.substr(query_loc);

		url = url.substr(0, query_loc);
	}

	size_t path_loc = url.find("/");
	if (path_loc != string::npos)
	{
		return_val.path = url.substr(path_loc);

		url = url.substr(0, path_loc);
	}

	// request is /[path][?query]
	return_val.request = return_val.path + return_val.query;

	size_t port_loc = url.find(":");
	if (port_loc != string::npos && port_loc + 1 < url.length())
	{
		// try converting port to an int
		string port_string = url.substr(port_loc + 1);
		try
		{
			return_val.port = stoi(port_string);

			// validate port range
			if (return_val.port <= Constants::MIN_PORT || return_val.port > Constants::MAX_PORT)
				throw out_of_range("");
		}
		// port was invalid
		catch (...)
		{
			return_val.status = 4; // invalid port code
			return return_val;
		}

		url = url.substr(0, port_loc);
	}
	// ':' was found at end of string
	else if (port_loc + 1 >= url.length())
	{
		return_val.status = 5; // missing port code
		return return_val;
	}

	// check remaining string for host portion
	if (url.length() > 0) 
	{
		return_val.host = url;
	}
	// no host specified
	else 
	{
		return_val.status = 6; // invalid host code
		return return_val;
	}

	return_val.status = 0;
	//printf("host %s, port %d\n", return_val.host.c_str(), return_val.port); // TODO: Remove prints
	return return_val;
}