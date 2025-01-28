#pragma once
#include <string>

class Movie {
public:
    std::string title;
    std::string year;
    std::string imdb_id;
    std::string actors;
    std::string poster_url;
    std::string plot;
    std::string rating;
    std::string director;
    std::string genre;
    std::string runtime;
    std::string type;        // movie, series, episode, etc.
    std::string released;  // Added for release date
	bool fetching = false;          // Flag to track if we're fetching details
    bool hasDetails = false;         // Flag to track if we've fetched full details
};