#pragma once
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <thread>
#include "Movie.h"

class MovieSearchService {
public:
    MovieSearchService();
    ~MovieSearchService();

    void searchMovies(const std::string& query,
        const std::string& year,
        const std::string& genre,
        bool exactMatch,
        std::function<void(const std::vector<Movie>&, const std::string&)> callback);

    void fetchMovieDetails(Movie& movie,
        std::function<void(const Movie&)> callback);

    bool isSearching() const { return m_isSearching; }

private:
    std::string encode_query(const std::string& query);
    bool checkGenreMatch(const std::string& movieGenre, const std::string& searchGenre);

    mutable std::mutex m_mutex;
    bool m_isSearching;
};