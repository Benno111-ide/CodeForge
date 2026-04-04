#include <iostream>
#include <vector>
#include <string>
#include <memory>

// Simple Game Engine Example
// Demonstrates inheritance, polymorphism, and smart pointers

class GameObject {
public:
    GameObject(const std::string& name, float x = 0.0f, float y = 0.0f) 
        : name(name), x(x), y(y), active(true) {}
    
    virtual ~GameObject() = default;
    
    virtual void update(float deltaTime) = 0;
    virtual void render() const = 0;
    
    // Getters and setters
    const std::string& getName() const { return name; }
    float getX() const { return x; }
    float getY() const { return y; }
    bool isActive() const { return active; }
    
    void setPosition(float newX, float newY) {
        x = newX;
        y = newY;
    }
    
    void setActive(bool state) { active = state; }

protected:
    std::string name;
    float x, y;
    bool active;
};

class Player : public GameObject {
private:
    float speed;
    int health;
    int score;

public:
    Player(const std::string& name, float x, float y) 
        : GameObject(name, x, y), speed(100.0f), health(100), score(0) {}
    
    void update(float deltaTime) override {
        // Simple movement (would be controlled by input in a real game)
        x += speed * deltaTime * 0.1f; // Move right slowly
        
        // Wrap around screen
        if (x > 800) x = 0;
    }
    
    void render() const override {
        std::cout << "Player [" << name << "] at (" << x << ", " << y 
                  << ") Health: " << health << " Score: " << score << std::endl;
    }
    
    void takeDamage(int damage) {
        health -= damage;
        if (health < 0) health = 0;
        std::cout << name << " takes " << damage << " damage! Health: " << health << std::endl;
    }
    
    void addScore(int points) {
        score += points;
        std::cout << name << " scored " << points << " points! Total: " << score << std::endl;
    }
    
    int getHealth() const { return health; }
    int getScore() const { return score; }
};

class Enemy : public GameObject {
private:
    float speed;
    int damage;

public:
    Enemy(const std::string& name, float x, float y) 
        : GameObject(name, x, y), speed(50.0f), damage(10) {}
    
    void update(float deltaTime) override {
        // Move left (towards player)
        x -= speed * deltaTime * 0.1f;
        
        // Deactivate if off-screen
        if (x < -50) {
            active = false;
        }
    }
    
    void render() const override {
        std::cout << "Enemy [" << name << "] at (" << x << ", " << y 
                  << ") Damage: " << damage << std::endl;
    }
    
    int getDamage() const { return damage; }
};

class Collectible : public GameObject {
private:
    int value;

public:
    Collectible(const std::string& name, float x, float y, int value) 
        : GameObject(name, x, y), value(value) {}
    
    void update(float deltaTime) override {
        // Rotate or animate (simple example)
        y += 10.0f * deltaTime * 0.1f;
        if ((int)(y) % 20 < 10) {
            y -= 20.0f * deltaTime * 0.1f;  // Bounce effect
        }
    }
    
    void render() const override {
        std::cout << "Collectible [" << name << "] at (" << x << ", " << y 
                  << ") Value: " << value << std::endl;
    }
    
    int getValue() const { return value; }
};

class GameEngine {
private:
    std::vector<std::unique_ptr<GameObject>> gameObjects;
    bool running;
    float gameTime;

public:
    GameEngine() : running(true), gameTime(0.0f) {}
    
    void addGameObject(std::unique_ptr<GameObject> obj) {
        gameObjects.push_back(std::move(obj));
    }
    
    void update(float deltaTime) {
        gameTime += deltaTime;
        
        // Update all game objects
        for (auto& obj : gameObjects) {
            if (obj->isActive()) {
                obj->update(deltaTime);
            }
        }
        
        // Remove inactive objects
        gameObjects.erase(
            std::remove_if(gameObjects.begin(), gameObjects.end(),
                [](const std::unique_ptr<GameObject>& obj) {
                    return !obj->isActive();
                }),
            gameObjects.end()
        );
        
        // Simple collision detection
        checkCollisions();
        
        // Spawn new enemies periodically
        if ((int)gameTime % 3 == 0 && (int)(gameTime * 10) % 30 == 0) {
            spawnEnemy();
        }
    }
    
    void render() const {
        std::cout << "\n=== Game State (Time: " << gameTime << "s) ===" << std::endl;
        
        for (const auto& obj : gameObjects) {
            if (obj->isActive()) {
                obj->render();
            }
        }
        
        std::cout << "Active objects: " << gameObjects.size() << std::endl;
    }
    
    void run() {
        const float deltaTime = 1.0f; // 1 second per frame for demo
        int frameCount = 0;
        
        std::cout << "Starting simple game engine demo..." << std::endl;
        
        while (running && frameCount < 10) { // Run for 10 frames
            update(deltaTime);
            render();
            
            frameCount++;
            
            // Check win/lose conditions
            bool hasPlayer = false;
            for (const auto& obj : gameObjects) {
                if (dynamic_cast<const Player*>(obj.get()) && obj->isActive()) {
                    const Player* player = static_cast<const Player*>(obj.get());
                    if (player->getHealth() <= 0) {
                        std::cout << "\nGame Over! Player defeated!" << std::endl;
                        running = false;
                    }
                    hasPlayer = true;
                    break;
                }
            }
            
            if (!hasPlayer) {
                std::cout << "\nGame Over! No player found!" << std::endl;
                running = false;
            }
            
            // Pause between frames for readability
            std::cout << "\n(Press Enter to continue...)" << std::endl;
            std::cin.get();
        }
        
        std::cout << "\nGame demo completed!" << std::endl;
    }

private:
    void checkCollisions() {
        Player* player = nullptr;
        
        // Find player
        for (auto& obj : gameObjects) {
            if (auto* p = dynamic_cast<Player*>(obj.get())) {
                player = p;
                break;
            }
        }
        
        if (!player) return;
        
        // Check collisions with other objects
        for (auto& obj : gameObjects) {
            if (obj.get() == player || !obj->isActive()) continue;
            
            float dx = player->getX() - obj->getX();
            float dy = player->getY() - obj->getY();
            float distance = std::sqrt(dx * dx + dy * dy);
            
            if (distance < 30.0f) { // Collision threshold
                if (auto* enemy = dynamic_cast<Enemy*>(obj.get())) {
                    player->takeDamage(enemy->getDamage());
                    obj->setActive(false); // Remove enemy after collision
                }
                else if (auto* collectible = dynamic_cast<Collectible*>(obj.get())) {
                    player->addScore(collectible->getValue());
                    obj->setActive(false); // Remove collectible after pickup
                }
            }
        }
    }
    
    void spawnEnemy() {
        static int enemyCount = 0;
        auto enemy = std::make_unique<Enemy>("Enemy_" + std::to_string(++enemyCount), 800, 100);
        addGameObject(std::move(enemy));
    }
};

int main() {
    std::cout << "=== Simple Game Engine Demo ===" << std::endl;
    std::cout << "Demonstrating object-oriented programming concepts:" << std::endl;
    std::cout << "- Inheritance (GameObject -> Player, Enemy, Collectible)" << std::endl;
    std::cout << "- Polymorphism (virtual functions)" << std::endl;
    std::cout << "- Smart pointers (std::unique_ptr)" << std::endl;
    std::cout << "- STL containers (std::vector)" << std::endl;
    std::cout << "\nStarting game..." << std::endl;
    
    GameEngine engine;
    
    // Create game objects
    auto player = std::make_unique<Player>("Hero", 100, 200);
    auto enemy = std::make_unique<Enemy>("Goblin", 400, 200);
    auto coin = std::make_unique<Collectible>("Gold Coin", 300, 150, 10);
    auto gem = std::make_unique<Collectible>("Ruby", 500, 180, 50);
    
    engine.addGameObject(std::move(player));
    engine.addGameObject(std::move(enemy));
    engine.addGameObject(std::move(coin));
    engine.addGameObject(std::move(gem));
    
    engine.run();
    
    return 0;
}