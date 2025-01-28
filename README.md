# IMDB Movie Search

A desktop application that allows users to search and browse movies using the OMDB (Open Movie Database) API. Built with C++ and Dear ImGui, this application provides a clean and efficient interface for searching movies, managing favorites, and viewing detailed movie information.

![IMDB Movie Search Screenshot](screenshot.png)

## Features

- **Search Functionality**
  - Search by movie title
  - Filter by year (1888-2024)
  - Filter by genre
  - Option for exact title matching
  - Asynchronous search with loading indicators

- **Movie Information**
  - Title and year
  - IMDB rating
  - Release date
  - Director
  - Genre
  - Runtime
  - Cast information
  - Plot summary
  - Direct links to IMDB page and movie poster

- **Organization Features**
  - Sort movies by various criteria (Title, Year, Rating, Released date, Runtime)
  - Toggle between ascending and descending sort order
  - Favorites system for saving preferred movies
  - Load saved favorites

- **User Interface**
  - Clean, modern interface built with Dear ImGui
  - Collapsible movie details
  - Dark/Light theme toggle
  - Scrollable results
  - Tooltips for better usability
  - Status messages with color coding

## OMDB API Integration

This project uses the [OMDB API](https://www.omdbapi.com/) for fetching movie data. To use this application:

1. Get your API key:
   - Visit [OMDB API Key Registration](https://www.omdbapi.com/apikey.aspx)
   - Choose Free or Paid tier
     - Free: 1,000 daily limit
     - Paid: Higher limits available

2. API Key Configuration:
   - Create a `config.h` file in the project root:
   ```cpp
   #define OMDB_API_KEY "your_api_key_here"
   ```
   - Or set it as an environment variable:
   ```bash
   export OMDB_API_KEY=your_api_key_here
   ```

3. API Features Used:
   - Search by title (`s=`)
   - Search by exact title (`t=`)
   - Get full movie details (`i=` with IMDB ID)
   - Year filtering (`y=`)

## Dependencies

- [Dear ImGui](https://github.com/ocornut/imgui)
- [cpp-httplib](https://github.com/yhirose/cpp-httplib)
- [nlohmann/json](https://github.com/nlohmann/json)
- Windows API (for opening URLs)

## Building the Project

### Prerequisites

- C++17 compatible compiler
- CMake 3.10 or higher
- Visual Studio 2019 or higher (for Windows)

### Build Steps

1. Clone the repository:
```bash
git clone https://github.com/yourusername/imdb-movie-search.git
cd imdb-movie-search
```

2. Configure your API key as described in the OMDB API Integration section

3. Create a build directory:
```bash
mkdir build
cd build
```

4. Generate build files:
```bash
cmake ..
```

5. Build the project:
```bash
cmake --build . --config Release
```

## Usage

1. Launch the application
2. Enter a movie title in the search box
3. (Optional) Add filters:
   - Year (4-digit year between 1888-2024)
   - Genre
   - Check "Exact Title" for precise matches
4. Click "Search" to find movies
5. Click on movie titles to expand and view details
6. Use the "Sort by" dropdown to organize results
7. Add/remove movies from favorites using the buttons in expanded view
8. Toggle between dark and light themes as needed

## Threading Model

The application uses asynchronous operations for:
- Movie searches
- Detail fetching
- Favorites management
- Sorting operations

This ensures the UI remains responsive during network operations and data processing.

## Error Handling

The application handles various API-related scenarios:
- Invalid API keys
- Network connectivity issues
- Rate limiting
- Invalid search parameters
- No results found

## Contributing

1. Fork the repository
2. Create a new branch for your feature
3. Commit your changes
4. Push to the branch
5. Create a new Pull Request

## Security Note

Never commit your API key to version control. The repository includes a `config.h` in the `.gitignore` file to prevent accidental commits of API keys.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [OMDB API](https://www.omdbapi.com/) for providing comprehensive movie data
- Dear ImGui for the UI framework
- cpp-httplib for HTTP requests
- nlohmann/json for JSON parsing
