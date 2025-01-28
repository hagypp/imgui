#include "movie_search_app.h"
#include "imgui.h"
#include <httplib.h>
#include <json.hpp>
#include <windows.h>

using json = nlohmann::json;

MovieSearchApp::MovieSearchApp()
    : isSearching(false), searchSingleMovie(false) {
    memset(searchBuffer, 0, sizeof(searchBuffer));
    memset(yearBuffer, 0, sizeof(yearBuffer));
    memset(genreBuffer, 0, sizeof(genreBuffer));
}

MovieSearchApp::~MovieSearchApp() = default;


int MovieSearchApp::extractRuntime(const std::string& runtime) {
    try {
        // Find the position of " min" and extract the numeric part before it
        size_t minPos = runtime.find(" min");
        if (minPos != std::string::npos) {
            std::string runtimeStr = runtime.substr(0, minPos);  // Get the numeric part
            return std::stoi(runtimeStr);  // Convert to integer and return
        }
    }
    catch (const std::invalid_argument&) {
        return 0;  // If conversion fails, return 0 (assumes invalid runtime)
    }
    return 0;  // Default if no valid runtime found
}

void MovieSearchApp::sortMovies() {
    // Launch the sorting in a separate thread to avoid blocking the UI
    std::thread sortThread([this]()
        {
        std::lock_guard<std::mutex> lock(movieMutex);  // Protect shared data with mutex

        switch (currentSortCriteria) {
        case SortCriteria::Title:
            std::sort(movies.begin(), movies.end(), [this](const Movie& a, const Movie& b) {
                return ascending ? a.title < b.title : a.title > b.title;
                });
            break;
        case SortCriteria::Year:
            std::sort(movies.begin(), movies.end(), [this](const Movie& a, const Movie& b) {
                try {
                    return ascending ? std::stoi(a.year) > std::stoi(b.year) : std::stoi(a.year) < std::stoi(b.year);
                }
                catch (const std::invalid_argument&) {
                    return false;  // Return false if conversion fails (invalid year)
                }
                });
            break;
        case SortCriteria::Rating:
            std::sort(movies.begin(), movies.end(), [this](const Movie& a, const Movie& b) {
                try {
                    return ascending ? std::stof(a.rating) > std::stof(b.rating) : std::stof(a.rating) < std::stof(b.rating);
                }
                catch (const std::invalid_argument&) {
                    return false;  // Return false if conversion fails (invalid rating)
                }
                });
            break;
        case SortCriteria::Released:
            std::sort(movies.begin(), movies.end(), [this](const Movie& a, const Movie& b) {
                // Extract the last 4 characters of the 'released' string to get the year
                std::string yearA = a.released.length() >= 4 ? a.released.substr(a.released.length() - 4) : "0000";
                std::string yearB = b.released.length() >= 4 ? b.released.substr(b.released.length() - 4) : "0000";
                return ascending ? yearA > yearB : yearA < yearB;  // Compare years as strings
                });
            break;
        case SortCriteria::Runtime:
            std::sort(movies.begin(), movies.end(), [this](const Movie& a, const Movie& b) {
                int runtimeA = extractRuntime(a.runtime);
                int runtimeB = extractRuntime(b.runtime);
                return ascending ? runtimeA > runtimeB : runtimeA < runtimeB;  // Sort by runtime
                });
            break;
        }
        });

    // Detach the thread to let it run independently
    sortThread.detach();
}

std::string MovieSearchApp::getSortCriteriaName(MovieSearchApp::SortCriteria criteria) {
    switch (criteria) {
    case SortCriteria::Title: return "Title";
    case SortCriteria::Year: return "Year";
    case SortCriteria::Rating: return "Rating";
    case SortCriteria::Released: return "Released";
    case SortCriteria::Runtime: return "Runtime";

    default: return "";
    }
}


void MovieSearchApp::render() 
{
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin("Movie Search", nullptr);

    // Search layout with title and year inputs
    ImGui::PushItemWidth(200); // Leave space for year input and search button
    if (ImGui::InputText("search", searchBuffer, sizeof(searchBuffer),
        ImGuiInputTextFlags_EnterReturnsTrue)) {
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Enter movie title to search");
    }
    ImGui::PopItemWidth();

    ImGui::SameLine();

    // Year input with validation
    ImGui::PushItemWidth(70);
    bool validYear = true;
    if (ImGui::InputText("Year", yearBuffer, sizeof(yearBuffer), ImGuiInputTextFlags_CharsDecimal)) //Optinal input year 
    {
        // Validate year input
        if (strlen(yearBuffer) > 0) {
            try {
                int year = std::stoi(yearBuffer);
                validYear = (year >= 1888 && year <= 2024 && strlen(yearBuffer) == 4);
                if (!validYear) {
                    memset(yearBuffer, 0, sizeof(yearBuffer));
                }
            }
            catch (...) {
                memset(yearBuffer, 0, sizeof(yearBuffer));
                validYear = false;
            }
        }
    }
    if (ImGui::IsItemHovered()) 
    {
        ImGui::SetTooltip("Optional: Enter 4-digit year (1888-2024)");
    }
	if (!validYear) // Show error icon if year is invalid
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "!");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Invalid year! Please enter a 4-digit year between 1888-2024");
        }
    }
    ImGui::PopItemWidth();

    ImGui::SameLine();
    ImGui::PushItemWidth(120);
	if (ImGui::InputText("Genre", genreBuffer, sizeof(genreBuffer))) // Optional genre input
    {
        // Convert input to title case for display
        if (strlen(genreBuffer) > 0) {
            genreBuffer[0] = toupper(genreBuffer[0]);
        }
    }
    if (ImGui::IsItemHovered()) 
    {
        ImGui::SetTooltip("Optional: Enter genre to filter results (e.g., Action, Drama, Comedy)");
    }
    ImGui::PopItemWidth();

    ImGui::SameLine();
	ImGui::Checkbox("Exact Title", &searchSingleMovie); // Optional checkbox for exact title search
    if (ImGui::IsItemHovered()) 
    {
        ImGui::SetTooltip("Optional: Search for exact movie title match");
    }

    ImGui::SameLine();
	if (ImGui::Button("Search") && !searchService.isSearching() && strlen(searchBuffer) > 0)  // Search button
    {
		statusMessage = "Searching...";
        searchService.searchMovies(
            searchBuffer,
            yearBuffer,
            genreBuffer,
            searchSingleMovie,
			[this](const std::vector<Movie>& results, const std::string& status)    // Callback lambda function done after search
            {
                std::lock_guard<std::mutex> lock(movieMutex);
                movies = results;
                statusMessage = status;
                sortMovies();
            });
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Search");
    }
    ImGui::PushItemWidth(70);


	// Sort options dropdown the moment the button is clicked it sort
    if (ImGui::BeginCombo("Sort by", getSortCriteriaName(currentSortCriteria).c_str())) 
    {
        if (ImGui::Selectable("Title", currentSortCriteria == SortCriteria::Title)) {
            currentSortCriteria = SortCriteria::Title;
            sortMovies();
        }
        if (ImGui::Selectable("Year", currentSortCriteria == SortCriteria::Year)) {
            currentSortCriteria = SortCriteria::Year;
            sortMovies();
        }
        if (ImGui::Selectable("Rating", currentSortCriteria == SortCriteria::Rating)) {
            currentSortCriteria = SortCriteria::Rating;
            sortMovies();
        }
        if (ImGui::Selectable("Released", currentSortCriteria == SortCriteria::Released)) {
            currentSortCriteria = SortCriteria::Released;
            sortMovies();
        }
        if (ImGui::Selectable("Runtime", currentSortCriteria == SortCriteria::Runtime)) {
            currentSortCriteria = SortCriteria::Runtime;
            sortMovies();
        }

        ImGui::EndCombo();
    }
	ImGui::SameLine();
    ImGui::PushItemWidth(70);

	if (ImGui::Button(ascending ? "Sort Descending" : "Sort Ascending")) // Sort direction button
    {
        ascending = !ascending;  // Toggle sorting direction
        sortMovies();  // Re-sort after toggling
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Sort by Descending / Ascending");
    }


	if (ImGui::Button("Load Favorites"))  // Load favorites button
    {
		statusMessage = "Loading favorites...";
        favorites.loadFavoritesAsync([this](const std::vector<Movie>& favMovies) 
            {
            std::lock_guard<std::mutex> lock(movieMutex);
            movies = favMovies;
            statusMessage = "Loaded " + std::to_string(movies.size()) + " favorite movies";
            });
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Load Favorites from the memory");
    }


	// Theme change button
    if (ImGui::Button(isDarkTheme ? "Light Theme" : "Dark Theme")) {
        isDarkTheme = !isDarkTheme;

        if (isDarkTheme) {
            ImGui::StyleColorsDark();
        }
        else {
            ImGui::StyleColorsLight();
        }
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Change the theme");
    }

	// Clear button
    if (ImGui::Button("clear"))
    {
        {
            std::lock_guard<std::mutex> lock(movieMutex);
            movies.clear();
        }
        memset(searchBuffer, 0, sizeof(searchBuffer));
        memset(yearBuffer, 0, sizeof(yearBuffer));
        memset(genreBuffer, 0, sizeof(genreBuffer));
        searchSingleMovie = false;
        statusMessage = "";
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Clear the screen");
    }


    // Status message with appropriate color
    if (!statusMessage.empty()) {
        ImGui::Spacing();
        if (statusMessage.find("Found") != std::string::npos)
        {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", statusMessage.c_str());
        }
        else if (statusMessage == "Searching...") {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", statusMessage.c_str());
        }
        else if (statusMessage.find("Loaded") != std::string::npos) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", statusMessage.c_str());
        }
        else if (statusMessage.find("Loading favorites...") != std::string::npos)
        {
             ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Loading favorites...");
        }
		else if (statusMessage.find("Update favorites") != std::string::npos)
		{
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Update favorites...");
		}
		else if (statusMessage.find("favorites updated") != std::string::npos)
		{
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "favorites updated");
		}
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", statusMessage.c_str());
        }
        ImGui::Separator();
    }


    {   //update the movies
        std::lock_guard<std::mutex> lock(movieMutex);

        if (!movies.empty())
        {
			ImGui::BeginChild("Results", ImVec2(0, 0), true);   // Begin scrolling region

            for (auto& movie : movies) 
            {
				std::string headerLabel = movie.title + " (" + movie.year + ")";    // Display title and year
                if (movie.type != "movie") {
                    headerLabel += " [" + movie.type + "]";
                }
                headerLabel += "##" + movie.imdb_id;

                bool header_open = ImGui::CollapsingHeader(headerLabel.c_str());

                // Load details automatically when header is opened
                if (header_open && !movie.hasDetails && !movie.fetching) 
                {
                    searchService.fetchMovieDetails(movie, [this](const Movie& updatedMovie) {});
                }

                if (header_open) 
                {
                    ImGui::Indent(20);

                    // Show loading indicator or details
                    if (!movie.hasDetails) {
                        ImGui::Text("Loading details...");
                    }
                    else
                    {
                        if (!movie.rating.empty() && movie.rating != "N/A")
                            ImGui::TextColored(ImVec4(1.0f, 0.843f, 0.0f, 1.0f), "Rating: %s", movie.rating.c_str());
                        if (!movie.released.empty() && movie.released != "N/A")
                            ImGui::TextColored(ImVec4(0.678f, 0.847f, 0.902f, 1.0f), "Released: %s", movie.released.c_str());
                        if (!movie.director.empty() && movie.director != "N/A")
                            ImGui::TextWrapped("Director: %s", movie.director.c_str());
                        if (!movie.genre.empty() && movie.genre != "N/A")
                            ImGui::TextWrapped("Genre: %s", movie.genre.c_str());
                        if (!movie.runtime.empty() && movie.runtime != "N/A")
                            ImGui::Text("Runtime: %s", movie.runtime.c_str());
                        ImGui::Spacing();
                        if (!movie.actors.empty() && movie.actors != "N/A")
                            ImGui::TextWrapped("Cast: %s", movie.actors.c_str());

                        if (!movie.plot.empty() && movie.plot != "N/A") {
                            ImGui::Spacing();
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
                            ImGui::TextWrapped("Plot: %s", movie.plot.c_str());
                            ImGui::PopStyleColor();
                        }
                    }

                    ImGui::Spacing();

                    std::string imdbButtonId = "IMDB Page##" + movie.imdb_id;
					if (ImGui::Button(imdbButtonId.c_str()))  // Open IMDB page button
                    {
                        std::string url = "https://imdb.com/title/" + movie.imdb_id;
                        ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
                    }

                    if (movie.poster_url != "N/A") {
                        ImGui::SameLine();
                        std::string posterButtonId = "View Poster##" + movie.imdb_id;
						if (ImGui::Button(posterButtonId.c_str()))  //Open poster button
                        {
                            ShellExecuteA(NULL, "open", movie.poster_url.c_str(), NULL, NULL, SW_SHOWNORMAL);
                        }
                    }

                    ImGui::SameLine();
                    std::string favButtonLabel = (favorites.isFavorite(movie.imdb_id) ?
                        "Remove from Favorites" : "Add to Favorites");
                    favButtonLabel += "##" + movie.imdb_id;
                    if (ImGui::Button(favButtonLabel.c_str()))  // Add/remove favorites button
                    {
						statusMessage = "Update favorites";
                        favorites.toggleFavorite(movie);
						statusMessage = "favorites updated";
                    }
                    ImGui::Unindent(20);
                    ImGui::Separator();
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip(movie.title.c_str());
                }
            }
            ImGui::EndChild();
        }
    }
    ImGui::End();
}