#include "MovieFavorites.h"

MovieFavorites::MovieFavorites(const std::string& filename)
    : favoritesFile(filename) {
}

MovieFavorites::~MovieFavorites() {
    while (saving) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void MovieFavorites::addFavorite(const Movie& movie) {
    {
        std::lock_guard<std::mutex> lock(favoritesMutex);
        if (!isFavorite(movie.imdb_id)) {
            favorites.push_back(movie);
        }
    }
    saveFavoritesAsync();
}

void MovieFavorites::removeFavorite(const std::string& imdb_id) {
    {
        std::lock_guard<std::mutex> lock(favoritesMutex);
        auto it = std::find_if(favorites.begin(), favorites.end(),
            [&imdb_id](const Movie& m) { return m.imdb_id == imdb_id; });

        if (it != favorites.end()) {
            favorites.erase(it);
        }
    }
    saveFavoritesAsync();
}

bool MovieFavorites::isFavorite(const std::string& imdb_id) const {
    return std::find_if(favorites.begin(), favorites.end(),
        [&imdb_id](const Movie& m) { return m.imdb_id == imdb_id; }) != favorites.end();
}

void MovieFavorites::loadFavoritesFromFile() {
    std::ifstream file(favoritesFile);
    if (file.is_open()) {
        try {
            json j = json::parse(file);
            std::lock_guard<std::mutex> lock(favoritesMutex);
            favorites.clear();

            for (const auto& movieJson : j) {
                Movie movie;
                movie.title = movieJson["title"];
                movie.year = movieJson["year"];
                movie.imdb_id = movieJson["imdb_id"];
                movie.poster_url = movieJson["poster_url"];
                movie.type = movieJson["type"];
                movie.plot = movieJson["plot"];
                movie.rating = movieJson["rating"];
                movie.actors = movieJson["actors"];
                movie.director = movieJson["director"];
                movie.genre = movieJson["genre"];
                movie.runtime = movieJson["runtime"];
                movie.released = movieJson["released"];
                movie.hasDetails = movieJson["hasDetails"];
                favorites.push_back(movie);
            }
        }
        catch (const std::exception&) {
            // Handle error if needed
        }
    }
}

void MovieFavorites::loadFavoritesAsync(std::function<void(const std::vector<Movie>&)> callback) {
    if (loading) return;

    loading = true;
    std::thread([this, callback]() {
        loadFavoritesFromFile();

        std::vector<Movie> moviesCopy;
        {
            std::lock_guard<std::mutex> lock(favoritesMutex);
            moviesCopy = favorites;
        }

        if (callback) {
            callback(moviesCopy);
        }

        loading = false;
        }).detach();
}

void MovieFavorites::saveFavoritesToFile() {
    json j = json::array();
    {
        std::lock_guard<std::mutex> lock(favoritesMutex);
        for (const auto& movie : favorites) {
            json movieJson = {
                {"title", movie.title},
                {"year", movie.year},
                {"imdb_id", movie.imdb_id},
                {"poster_url", movie.poster_url},
                {"type", movie.type},
                {"plot", movie.plot},
                {"rating", movie.rating},
                {"actors", movie.actors},
                {"director", movie.director},
                {"genre", movie.genre},
                {"runtime", movie.runtime},
                {"released", movie.released},
                {"hasDetails", movie.hasDetails}
            };
            j.push_back(movieJson);
        }
    }

    std::ofstream file(favoritesFile);
    if (file.is_open()) {
        file << j.dump(4);
    }
}

void MovieFavorites::saveFavoritesAsync() {
    if (saving) return;

    saving = true;
    std::thread([this]() {
        saveFavoritesToFile();
        saving = false;
        }).detach();
}

void MovieFavorites::toggleFavorite(const Movie& movie) {
    if (isFavorite(movie.imdb_id)) {
        removeFavorite(movie.imdb_id);
    }
    else {
        addFavorite(movie);
    }
}

size_t MovieFavorites::getFavoritesCount() const {
    std::lock_guard<std::mutex> lock(favoritesMutex);
    return favorites.size();
}
