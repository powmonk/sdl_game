#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <vector>
#include <string>
#include <memory>
#include <fstream>
#include <map>

// Represents a single tile in our map
class Tile {
public:
    enum TileType {
        EMPTY = 0,
        SOLID,
        PLATFORM,
        LADDER,
        WATER,
        HAZARD
    };

    Tile(int id, TileType type) : m_id(id), m_type(type) {}
    
    int getId() const { return m_id; }
    TileType getType() const { return m_type; }
    
private:
    int m_id;
    TileType m_type;
};

// Manages our tileset (collection of tile images)
class TileSet {
public:
    TileSet(SDL_Renderer* renderer, const std::string& tilesetPath, int tileWidth, int tileHeight) 
        : m_tileWidth(tileWidth), m_tileHeight(tileHeight) {
        m_texture = IMG_LoadTexture(renderer, tilesetPath.c_str());
        if (!m_texture) {
            printf("Failed to load tileset: %s\n", IMG_GetError());
            return;
        }
        
        // Get the dimensions of the loaded tileset
        int w, h;
        SDL_QueryTexture(m_texture, nullptr, nullptr, &w, &h);
        
        // Calculate number of tiles in the tileset
        m_columns = w / tileWidth;
        m_rows = h / tileHeight;
    }
    
    ~TileSet() {
        if (m_texture) {
            SDL_DestroyTexture(m_texture);
        }
    }
    
    void renderTile(SDL_Renderer* renderer, int tileId, int x, int y) {
        if (!m_texture || tileId < 0 || tileId >= m_columns * m_rows) {
            return;
        }
        
        // Calculate tile position in the tileset
        int srcX = (tileId % m_columns) * m_tileWidth;
        int srcY = (tileId / m_columns) * m_tileHeight;
        
        SDL_Rect srcRect = { srcX, srcY, m_tileWidth, m_tileHeight };
        SDL_Rect dstRect = { x, y, m_tileWidth, m_tileHeight };
        
        SDL_RenderCopy(renderer, m_texture, &srcRect, &dstRect);
    }
    
    int getTileWidth() const { return m_tileWidth; }
    int getTileHeight() const { return m_tileHeight; }
    
private:
    SDL_Texture* m_texture = nullptr;
    int m_tileWidth;
    int m_tileHeight;
    int m_columns;
    int m_rows;
};

// A single layer in our map
class MapLayer {
public:
    MapLayer(int width, int height) : m_width(width), m_height(height) {
        m_tileIds.resize(width * height, -1);
    }
    
    void setTile(int x, int y, int tileId) {
        if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
            m_tileIds[y * m_width + x] = tileId;
        }
    }
    
    int getTile(int x, int y) const {
        if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
            return m_tileIds[y * m_width + x];
        }
        return -1;
    }
    
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    
private:
    std::vector<int> m_tileIds;
    int m_width;
    int m_height;
};

// Main tile map class that manages all layers and collision data
class TileMap {
public:
    TileMap(SDL_Renderer* renderer, const std::string& tilesetPath, int tileWidth, int tileHeight) 
        : m_tileWidth(tileWidth), m_tileHeight(tileHeight) {
        m_tileset = std::make_unique<TileSet>(renderer, tilesetPath, tileWidth, tileHeight);
    }
    
    // Add a new layer to the map
    void addLayer(int width, int height, float parallaxFactor = 1.0f) {
        m_layers.push_back(std::make_unique<MapLayer>(width, height));
        m_parallaxFactors.push_back(parallaxFactor);
        
        // Update map dimensions if needed
        m_mapWidth = std::max(m_mapWidth, width);
        m_mapHeight = std::max(m_mapHeight, height);
    }
    
    // Set a tile in a specific layer
    void setTile(int layerIndex, int x, int y, int tileId) {
        if (layerIndex >= 0 && layerIndex < m_layers.size()) {
            m_layers[layerIndex]->setTile(x, y, tileId);
            
            // If this is the main collision layer (layer 0 by convention),
            // update the collision data
            if (layerIndex == 0 && tileId >= 0) {
                m_collisionTypes[y * m_mapWidth + x] = m_tileCollisionMap[tileId];
            }
        }
    }
    
    // Set collision type for a specific tile ID
    void setTileCollisionType(int tileId, Tile::TileType type) {
        m_tileCollisionMap[tileId] = type;
    }
    
    // Check if a point collides with a solid tile
    bool collidesWithSolid(float x, float y) const {
        int tileX = static_cast<int>(x) / m_tileWidth;
        int tileY = static_cast<int>(y) / m_tileHeight;
        
        if (tileX < 0 || tileX >= m_mapWidth || tileY < 0 || tileY >= m_mapHeight) {
            return false;  // Outside map bounds
        }
        
        auto it = m_collisionTypes.find(tileY * m_mapWidth + tileX);
        if (it != m_collisionTypes.end()) {
            return it->second == Tile::SOLID;
        }
        
        return false;
    }
    
    // Get collision type at a specific world position
    Tile::TileType getCollisionTypeAt(float x, float y) const {
        int tileX = static_cast<int>(x) / m_tileWidth;
        int tileY = static_cast<int>(y) / m_tileHeight;
        
        if (tileX < 0 || tileX >= m_mapWidth || tileY < 0 || tileY >= m_mapHeight) {
            return Tile::EMPTY;  // Outside map bounds
        }
        
        auto it = m_collisionTypes.find(tileY * m_mapWidth + tileX);
        if (it != m_collisionTypes.end()) {
            return it->second;
        }
        
        return Tile::EMPTY;
    }
    
    // Render the visible portion of the map
    void render(SDL_Renderer* renderer, int cameraX, int cameraY, int screenWidth, int screenHeight) {
        // Calculate the range of tiles that are visible on screen
        int startTileX = cameraX / m_tileWidth;
        int startTileY = cameraY / m_tileHeight;
        int endTileX = (cameraX + screenWidth) / m_tileWidth + 1;
        int endTileY = (cameraY + screenHeight) / m_tileHeight + 1;
        
        // Render each layer
        for (size_t layer = 0; layer < m_layers.size(); ++layer) {
            float parallaxFactor = m_parallaxFactors[layer];
            
            // Adjust camera position based on parallax factor
            int layerCameraX = static_cast<int>(cameraX * parallaxFactor);
            int layerCameraY = static_cast<int>(cameraY * parallaxFactor);
            
            // Recalculate visible tiles for this layer
            int layerStartTileX = layerCameraX / m_tileWidth;
            int layerStartTileY = layerCameraY / m_tileHeight;
            int layerEndTileX = (layerCameraX + screenWidth) / m_tileWidth + 1;
            int layerEndTileY = (layerCameraY + screenHeight) / m_tileHeight + 1;
            
            // Clamp to map bounds
            layerStartTileX = std::max(0, layerStartTileX);
            layerStartTileY = std::max(0, layerStartTileY);
            layerEndTileX = std::min(m_layers[layer]->getWidth(), layerEndTileX);
            layerEndTileY = std::min(m_layers[layer]->getHeight(), layerEndTileY);
            
            // Render visible tiles
            for (int y = layerStartTileY; y < layerEndTileY; ++y) {
                for (int x = layerStartTileX; x < layerEndTileX; ++x) {
                    int tileId = m_layers[layer]->getTile(x, y);
                    if (tileId >= 0) {
                        int screenX = x * m_tileWidth - layerCameraX;
                        int screenY = y * m_tileHeight - layerCameraY;
                        m_tileset->renderTile(renderer, tileId, screenX, screenY);
                    }
                }
            }
        }
    }
    
    // Load map from a simple file format
    bool loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            printf("Failed to open map file: %s\n", filename.c_str());
            return false;
        }
        
        // Read map dimensions and layers
        int width, height, numLayers;
        file >> width >> height >> numLayers;
        
        // Clear existing map data
        m_layers.clear();
        m_parallaxFactors.clear();
        m_collisionTypes.clear();
        
        m_mapWidth = width;
        m_mapHeight = height;
        
        // Read layers
        for (int i = 0; i < numLayers; ++i) {
            float parallaxFactor;
            file >> parallaxFactor;
            
            addLayer(width, height, parallaxFactor);
            
            // Read tile IDs for this layer
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    int tileId;
                    file >> tileId;
                    setTile(i, x, y, tileId);
                }
            }
        }
        
        return true;
    }
    
    // Save map to a file
    bool saveToFile(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            printf("Failed to create map file: %s\n", filename.c_str());
            return false;
        }
        
        // Write map dimensions and layers
        file << m_mapWidth << " " << m_mapHeight << " " << m_layers.size() << std::endl;
        
        // Write layers
        for (size_t i = 0; i < m_layers.size(); ++i) {
            file << m_parallaxFactors[i] << std::endl;
            
            // Write tile IDs for this layer
            for (int y = 0; y < m_layers[i]->getHeight(); ++y) {
                for (int x = 0; x < m_layers[i]->getWidth(); ++x) {
                    file << m_layers[i]->getTile(x, y) << " ";
                }
                file << std::endl;
            }
        }
        
        return true;
    }
    
    int getMapWidth() const { return m_mapWidth * m_tileWidth; }
    int getMapHeight() const { return m_mapHeight * m_tileHeight; }
    
private:
    std::vector<std::unique_ptr<MapLayer>> m_layers;
    std::vector<float> m_parallaxFactors;
    std::unique_ptr<TileSet> m_tileset;
    std::map<int, Tile::TileType> m_tileCollisionMap;  // Maps tile IDs to collision types
    std::map<int, Tile::TileType> m_collisionTypes;    // Maps tile positions to collision types
    int m_tileWidth;
    int m_tileHeight;
    int m_mapWidth = 0;
    int m_mapHeight = 0;
};

// Camera class to handle scrolling
class Camera {
public:
    Camera(int mapWidth, int mapHeight, int screenWidth, int screenHeight)
        : m_mapWidth(mapWidth), m_mapHeight(mapHeight), 
          m_screenWidth(screenWidth), m_screenHeight(screenHeight) {
        m_x = 0;
        m_y = 0;
    }
    
    void centerOn(float x, float y) {
        // Center the camera on the target position
        m_x = x - m_screenWidth / 2;
        m_y = y - m_screenHeight / 2;
        
        // Clamp to map bounds
        m_x = std::max(0.0f, std::min(static_cast<float>(m_mapWidth - m_screenWidth), m_x));
        m_y = std::max(0.0f, std::min(static_cast<float>(m_mapHeight - m_screenHeight), m_y));
    }
    
    float getX() const { return m_x; }
    float getY() const { return m_y; }
    
private:
    float m_x;
    float m_y;
    int m_mapWidth;
    int m_mapHeight;
    int m_screenWidth;
    int m_screenHeight;
};

// Example usage in a game class
class Game {
public:
    Game(int screenWidth, int screenHeight) 
        : m_screenWidth(screenWidth), m_screenHeight(screenHeight) {
        // Initialize SDL
        SDL_Init(SDL_INIT_VIDEO);
        IMG_Init(IMG_INIT_PNG);
        
        m_window = SDL_CreateWindow("2D Sidescroller", 
                                  SDL_WINDOWPOS_UNDEFINED, 
                                  SDL_WINDOWPOS_UNDEFINED, 
                                  screenWidth, screenHeight, 0);
        m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
        
        // Initialize map
        m_map = std::make_unique<TileMap>(m_renderer, "tileset.png", 32, 32);
        
        // Create or load a map
        createExampleMap();
        // Or: m_map->loadFromFile("level1.map");
        
        // Initialize camera
        m_camera = std::make_unique<Camera>(m_map->getMapWidth(), m_map->getMapHeight(), 
                                          screenWidth, screenHeight);
    }
    
    ~Game() {
        SDL_DestroyRenderer(m_renderer);
        SDL_DestroyWindow(m_window);
        IMG_Quit();
        SDL_Quit();
    }
    
    void createExampleMap() {
        // Create a simple map with 3 layers: background, main, foreground
        m_map->addLayer(100, 20, 0.5f);  // Background with parallax factor 0.5
        m_map->addLayer(100, 20, 1.0f);  // Main gameplay layer
        m_map->addLayer(100, 20, 1.2f);  // Foreground with parallax factor 1.2
        
        // Set collision types for different tiles
        m_map->setTileCollisionType(1, Tile::SOLID);  // Ground tiles
        m_map->setTileCollisionType(2, Tile::PLATFORM);  // Platform tiles
        m_map->setTileCollisionType(5, Tile::HAZARD);  // Hazard tiles
        
        // Add some ground tiles
        for (int x = 0; x < 100; ++x) {
            m_map->setTile(1, x, 15, 1);  // Ground level
        }
        
        // Add some platforms
        for (int x = 10; x < 15; ++x) {
            m_map->setTile(1, x, 12, 2);
        }
        
        // Add some background details
        for (int x = 5; x < 20; ++x) {
            m_map->setTile(0, x, 5, 10);  // Some clouds or mountains
        }
    }
    
    void run() {
        bool quit = false;
        SDL_Event event;
        
        // Game loop
        while (!quit) {
            // Handle events
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    quit = true;
                }
                // Handle other game input...
            }
            
            // Update game state (player, entities, etc.)
            update();
            
            // Clear screen
            SDL_SetRenderDrawColor(m_renderer, 100, 150, 255, 255);
            SDL_RenderClear(m_renderer);
            
            // Render the map
            m_map->render(m_renderer, m_camera->getX(), m_camera->getY(), 
                        m_screenWidth, m_screenHeight);
            
            // Render other game entities...
            
            // Present the frame
            SDL_RenderPresent(m_renderer);
            
            // Cap to 60 FPS
            SDL_Delay(16);
        }
    }
    
    void update() {
        // Update player position based on input and physics...
        float playerX = 100.0f;  // Example fixed position for demo
        float playerY = 300.0f;
        
        // Center camera on player
        m_camera->centerOn(playerX, playerY);
        
        // Example collision detection
        bool onGround = m_map->collidesWithSolid(playerX, playerY + 32);
        Tile::TileType tileUnderPlayer = m_map->getCollisionTypeAt(playerX, playerY + 32);
        
        // Handle collision responses...
    }
    
private:
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    std::unique_ptr<TileMap> m_map;
    std::unique_ptr<Camera> m_camera;
    int m_screenWidth;
    int m_screenHeight;
};

int main(int argc, char* argv[]) {
    Game game(800, 600);
    game.run();
    return 0;
}