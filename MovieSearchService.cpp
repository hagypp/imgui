#include "MovieSearchService.h"
#include <httplib.h>
#include <json.hpp>
#include <iostream>

using json = nlohmann::json;

MovieSearchService::MovieSearchService() : m_isSearching(false) {}

MovieSearchService::~MovieSearchService() = default;

std::string MovieSearchService::encode_query(const std::string& query)  //encode the query
{
    std::string encoded;
    for (char c : query) {
        if (isalnum(c) || c == '-') {
            encoded += c;
        }
        else if (c == ' ') {
            encoded += '+';
        }
        else {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", static_cast<unsigned char>(c));
            encoded += buf;
        }
    }
    return encoded;
}

bool MovieSearchService::checkGenreMatch(const std::string& movieGenre, const std::string& searchGenre) //check if the genre match
{
    if (searchGenre.empty()) return true;

    std::string searchGenreLower = searchGenre;
    std::string movieGenreLower = movieGenre;
    std::transform(searchGenreLower.begin(), searchGenreLower.end(),
        searchGenreLower.begin(), ::tolower);
    std::transform(movieGenreLower.begin(), movieGenreLower.end(),
        movieGenreLower.begin(), ::tolower);

    return movieGenreLower.find(searchGenreLower) != std::string::npos;
}

void MovieSearchService::searchMovies(
    const std::string& query,
    const std::string& year,
    const std::string& genre,
    bool exactMatch,
    std::function<void(const std::vector<Movie>&, const std::string&)> callback)
{
    if (query.empty()) {
        callback({}, "Empty search query");
        return;
    }

    std::thread([this, query, year, genre, exactMatch, callback]() {
        std::vector<Movie> results;
        std::string status;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_isSearching = true;
            status = "Searching...";
        }

		httplib::Client cli("www.omdbapi.com"); //connect to the api domain
        std::string searchUrl = "/?apikey=fb4a2231&" +
			std::string(exactMatch ? "t=" : "s=") + encode_query(query);    //search the query

		if (year.length() == 4) //search the year
        {
            searchUrl += "&y=" + year;
        }

        auto res = cli.Get(searchUrl.c_str());

        if (res && res->status == 200) {
            try {
                std::cout << res->body << std::endl;
                auto j = json::parse(res->body);
                if (j["Response"] == "True") {
                    if (exactMatch) 
                    {
                        Movie movie;
                        movie.title = j.value("Title", "Unknown Title");
                        movie.year = j.value("Year", "N/A");
                        movie.imdb_id = j.value("imdbID", "");
                        movie.poster_url = j.value("Poster", "N/A");
                        movie.type = j.value("Type", "unknown");
                        movie.plot = j.value("Plot", "N/A");
                        movie.rating = j.value("imdbRating", "N/A");
                        movie.actors = j.value("Actors", "N/A");
                        movie.director = j.value("Director", "N/A");
                        movie.genre = j.value("Genre", "N/A");
                        movie.runtime = j.value("Runtime", "N/A");
                        movie.released = j.value("Released", "N/A");
                        movie.hasDetails = true;

                        if (checkGenreMatch(movie.genre, genre)) {
                            results.push_back(movie);
                        }
                    }
                    else 
                    {
                        auto search_results = j["Search"];
                        for (const auto& item : search_results) {
                            Movie movie;
                            movie.title = item.value("Title", "Unknown Title");
                            movie.year = item.value("Year", "N/A");
                            movie.imdb_id = item.value("imdbID", "");
                            movie.poster_url = item.value("Poster", "N/A");
                            movie.type = item.value("Type", "unknown");
                            movie.hasDetails = false;

							if (!genre.empty()) //search the full details
                            {
                                httplib::Client detail_cli("www.omdbapi.com");
                                auto detail_res = detail_cli.Get(
                                    ("/?apikey=fb4a2231&i=" + movie.imdb_id + "&plot=full").c_str());

                                if (detail_res && detail_res->status == 200) {
                                    auto detail_j = json::parse(detail_res->body);
                                    if (detail_j["Response"] == "True") {
                                        movie.genre = detail_j.value("Genre", "N/A");
                                        if (checkGenreMatch(movie.genre, genre)) {
                                            movie.plot = detail_j.value("Plot", "N/A");
                                            movie.rating = detail_j.value("imdbRating", "N/A");
                                            movie.actors = detail_j.value("Actors", "N/A");
                                            movie.director = detail_j.value("Director", "N/A");
                                            movie.runtime = detail_j.value("Runtime", "N/A");
                                            movie.released = detail_j.value("Released", "N/A");
                                            movie.hasDetails = true;
                                            results.push_back(movie);
                                        }
                                    }
                                }
                            }
                            else {
                                results.push_back(movie);
                            }
                        }
                    }
                    status = "Found " + std::to_string(results.size()) + " results";
                }
                else {
                    status = "No results found";
                }
            }
            catch (const std::exception& e) {
                status = "Error parsing response: " + std::string(e.what());
            }
        }
        else {
            status = "Request failed!";
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_isSearching = false;
        }

        callback(results, status);
        }).detach();
}

void MovieSearchService::fetchMovieDetails(Movie& movie,std::function<void(const Movie&)> callback)
{ //fetch the movie details

    if (movie.hasDetails || movie.fetching) return;

    std::thread([this, &movie, callback]() 
     {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            movie.fetching = true;
        }

        httplib::Client cli("www.omdbapi.com");
        auto res = cli.Get(("/?apikey=fb4a2231&i=" + movie.imdb_id + "&plot=full").c_str());

        if (res && res->status == 200) {
            try {
                std::cout << res->body << std::endl;

                auto j = json::parse(res->body);
                if (j["Response"] == "True") {
                    movie.plot = j.value("Plot", "N/A");
                    movie.rating = j.value("imdbRating", "N/A");
                    movie.actors = j.value("Actors", "N/A");
                    movie.director = j.value("Director", "N/A");
                    movie.genre = j.value("Genre", "N/A");
                    movie.runtime = j.value("Runtime", "N/A");
                    movie.released = j.value("Released", "N/A");
                    movie.hasDetails = true;
                }
            }
            catch (const std::exception& e) {
                std::cout << "Error fetching details: " << e.what() << std::endl;
            }
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            movie.fetching = false;
        }

        callback(movie);
        }).detach();
}