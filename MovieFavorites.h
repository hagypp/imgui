#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <json.hpp>
#include <thread>
#include <future>
#include <mutex>
#include "Movie.h"

using json = nlohmann::json;

class MovieFavorites {
public:
    MovieFavorites(const std::string& filename = "favorites.json");
    ~MovieFavorites();

    void addFavorite(const Movie& movie);
    void removeFavorite(const std::string& imdb_id);
    bool isFavorite(const std::string& imdb_id) const;
    void loadFavoritesAsync(std::function<void(const std::vector<Movie>&)> callback);
    void saveFavoritesAsync();
    void toggleFavorite(const Movie& movie);
    size_t getFavoritesCount() const;
    bool isLoading() const { return loading; }
    bool isSaving() const { return saving; }

private:
    std::string favoritesFile;
    std::vector<Movie> favorites;
    mutable std::mutex favoritesMutex;
    std::atomic<bool> loading{ false };
    std::atomic<bool> saving{ false };

    void loadFavoritesFromFile();
    void saveFavoritesToFile();
};
