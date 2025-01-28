#pragma once
#include <vector>
#include <string>
#include <memory>
#include "movie.h"
#include <mutex>
#include "MovieFavorites.h"
#include "MovieSearchService.h"
#include "imgui.h"

class MovieSearchApp {
public:
    MovieSearchApp();   //constractur
    ~MovieSearchApp();
    void render();

    enum class SortCriteria 
    {
        Title,
        Year,
        Rating,
        Released,
        Runtime
    };
    SortCriteria currentSortCriteria = SortCriteria::Title; // Default sort by Title

    bool ascending = true;  // true for ascending, false for descending

    bool isDarkTheme = true;


private:
    // Mutex to protect shared resources
    std::mutex movieMutex;

    void sortMovies();
	int extractRuntime(const std::string& runtime);
	std::string getSortCriteriaName(SortCriteria criteria);


    std::string ApiKey = "fb4a2231";

    std::vector<Movie> movies;  //vector that save the current movies

    //search inputs
    char searchBuffer[256];
    char yearBuffer[5];  // Added for year input (4 digits + null terminator)
    char genreBuffer[128];
    bool searchSingleMovie;

    bool isSearching;

    std::string statusMessage;

    //service for save favorites
    MovieFavorites favorites;
    //service for do search
    MovieSearchService searchService;
};
