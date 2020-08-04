#include <SFML/Graphics.hpp>
#include <iostream>
#include <memory>
#include <random>
#include <ctime>

///// DICE ROLL GENERATOR
int rollTheDice(int valmin, int valmax)
{
    static std::mt19937 generator{static_cast<unsigned>(time(nullptr))};
    std::uniform_int_distribution<> dist{valmin, valmax};

    return dist(generator);
}

/*
====================================================================
            ENTITY
====================================================================
*/

class Entity : public sf::RectangleShape
{
    const float SPEED = 10.f;

public:
    Entity(const sf::Vector2f& size, const sf::Vector2f& position);

public:
    inline sf::Vector2f getCenter() const
    {
        const sf::FloatRect boundary = getGlobalBounds();
        return sf::Vector2f{
            boundary.left + (boundary.width/2.f),
            boundary.top + (boundary.height/2.f)
        };
    }

    inline void move(float dt)
    {
        const auto pos = getPosition();
        const float width = getGlobalBounds().width;
        const float height = getGlobalBounds().height;
        if(pos.x <= 0.f) m_mouvement.x *= -1.f;
        if(pos.x >= (1600.f - width)) m_mouvement.x *= -1.f;
        if(pos.y <= 0.f) m_mouvement.y *= -1.f;
        if(pos.y >= (900.f - height)) m_mouvement.y *= -1.f;
        sf::RectangleShape::move(m_mouvement.x * dt * SPEED, m_mouvement.y * dt * SPEED);
    }

    inline void changeDir()
    {
        m_mouvement.x = rollTheDice(-10, 10);
        m_mouvement.y = rollTheDice(-10, 10);
    }

private:
    sf::Vector2f m_mouvement;
};

Entity::Entity(const sf::Vector2f& size, const sf::Vector2f& position):
    sf::RectangleShape{size}
{
    setPosition(position);
    m_mouvement.x = rollTheDice(-10, 10);
    m_mouvement.y = rollTheDice(-10, 10);
}

/*
====================================================================
            QUADTREE
====================================================================
*/

class Quadtree;
using Quadtree_p = std::unique_ptr<Quadtree>;

class Quadtree : public sf::Drawable
{
    const unsigned int NODE_CAPACITY = 5;

public:
    Quadtree() = delete;
    Quadtree(const sf::FloatRect& boundary);
    Quadtree(const Quadtree&) = delete;
    Quadtree(Quadtree&&) = delete;
    ~Quadtree() = default;

public:
    bool insert(const Entity* entity);
    void queryEntities(std::vector<const Entity*>& entitiesInRange, const sf::FloatRect& range);

private:
    void subdivide();
    // Override from sf::Drawable
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const final;

private:
    sf::FloatRect m_boundary;
    std::vector<const Entity*> m_entities;
    bool m_divided = false;
    Quadtree_p m_northwest = nullptr;
    Quadtree_p m_northeast = nullptr;
    Quadtree_p m_southwest = nullptr;
    Quadtree_p m_southeast = nullptr;
    sf::RectangleShape m_rect; // Only for drawing
};

Quadtree::Quadtree(const sf::FloatRect& boundary):
    m_boundary{boundary}
{
    m_rect.setSize({m_boundary.width, m_boundary.height});
    m_rect.setPosition(m_boundary.left, m_boundary.top);
    m_rect.setOutlineThickness(1);
    m_rect.setOutlineColor(sf::Color{60, 60, 60});
}

bool Quadtree::insert(const Entity* entity)
{
    const sf::Vector2f center = entity->getCenter();
    if(!m_boundary.contains(center)){
        return false;
    }

    if(m_entities.size() < NODE_CAPACITY && !m_divided){
        m_entities.push_back(entity);
        return true;
    }

    if(!m_divided){
        subdivide();
    }

    if(m_northwest->insert(entity)) return true;
    if(m_northeast->insert(entity)) return true;
    if(m_southwest->insert(entity)) return true;
    if(m_southeast->insert(entity)) return true;

    return false;
}

void Quadtree::queryEntities(std::vector<const Entity*>& entitiesInRange, const sf::FloatRect& range)
{
    if(!m_boundary.intersects(range)) return;

    for(auto&& e : m_entities){
        if(range.contains(e->getCenter())){
            entitiesInRange.emplace_back(e);
        }
    }

    if(!m_divided) return;

    m_northwest->queryEntities(entitiesInRange, range);
    m_northeast->queryEntities(entitiesInRange, range);
    m_southwest->queryEntities(entitiesInRange, range);
    m_southeast->queryEntities(entitiesInRange, range);
}

void Quadtree::subdivide()
{
    const float halfW = m_boundary.width / 2.f;
    const float halfH = m_boundary.height / 2.f;

    sf::FloatRect nwb{{m_boundary.left, m_boundary.top}, {halfW, halfH}};
    sf::FloatRect neb{{m_boundary.left + halfW, m_boundary.top}, {halfW, halfH}};
    sf::FloatRect swb{{m_boundary.left, m_boundary.top + halfH}, {halfW, halfH}};
    sf::FloatRect seb{{m_boundary.left + halfW, m_boundary.top + halfH}, {halfW, halfH}};

    m_northwest = std::make_unique<Quadtree>(nwb);
    m_northeast = std::make_unique<Quadtree>(neb);
    m_southwest = std::make_unique<Quadtree>(swb);
    m_southeast = std::make_unique<Quadtree>(seb);

    for(auto&& e : m_entities){
        if(m_northwest->insert(e)) continue;
        if(m_northeast->insert(e)) continue;
        if(m_southwest->insert(e)) continue;
        if(m_southeast->insert(e)) continue;
    }

    m_entities.clear();

    m_divided = true;
}

// Override from sf::Drawable
void Quadtree::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(m_rect);
    if(m_divided){
        target.draw(*m_northwest);
        target.draw(*m_northeast);
        target.draw(*m_southwest);
        target.draw(*m_southeast);
    }
}

/*
====================================================================
            HELPERS
====================================================================
*/

unsigned int askEntitiesNumber()
{
    std::cout << "How much entities you need ?\n";
    unsigned int entitiesNumber = 0;
    std::cin >> entitiesNumber;
    
    return entitiesNumber;
}

bool askDetectionMethod()
{
    std::cout << "Activate quadtree detection ? (y/n)\n";
    bool quadtreeDetection = true;
    char answer = 'y';
    std::cin >> answer;
    if(answer == 'n') quadtreeDetection = false;
    
    return quadtreeDetection;
}

void createEntity(std::vector<Entity>& entities, const sf::RenderWindow& window)
{
    sf::Vector2i mPos = sf::Mouse::getPosition(window);
    float x = rollTheDice(mPos.x - 32.f, mPos.x + 32.f);
    float y = rollTheDice(mPos.y - 32.f, mPos.y + 32.f);
    Entity e{{2.f, 2.f}, {x, y}};
    e.setFillColor(sf::Color::Blue);
    entities.emplace_back(e);
}

void createEntities(std::vector<Entity>& entities, const unsigned int totalEntities)
{
    for(unsigned int i = 0; i < totalEntities; i++){
        float x = rollTheDice(50, 1000);
        float y = rollTheDice(50, 400);
        Entity e{{8.f, 8.f}, {x, y}};
        e.setFillColor(sf::Color::Blue);
        entities.emplace_back(e);
    }
}

bool isCollide(const Entity* a, const Entity* b)
{
    const sf::FloatRect aBounds = a->getGlobalBounds();
    const sf::FloatRect bBounds = b->getGlobalBounds();
    return aBounds.intersects(bBounds);
}

/*
====================================================================
            MAIN
====================================================================
*/
int main()
{
    const unsigned int totalEntities = askEntitiesNumber();
    const bool quadtreeDetection = askDetectionMethod();
    
    const unsigned int WINDOW_W = 1600;
    const unsigned int WINDOW_H = 900;

    sf::RenderWindow window{{WINDOW_W, WINDOW_H}, "Quadtree 2D Collision"};

    sf::Font font{};
    if(!font.loadFromFile("arial.ttf")) // Windows
	font.loadFromFile("FreeMono.ttf"); // Linux
    sf::Text fps{"0", font};
    fps.setCharacterSize(30);
    fps.setStyle(sf::Text::Bold);
    fps.setFillColor(sf::Color::Green);
    fps.setPosition(1600.f - 80.f, 10.f);
    fps.setOutlineThickness(1);
    fps.setOutlineColor(sf::Color::Black);
    float timeSinceFPSUpdate = 0.f;

    std::vector<Entity> entities{};
    createEntities(entities, totalEntities);

    std::vector<const Entity*> entitiesInRange{};

    bool mousePressed = false;
    sf::Clock clock{};
    sf::Time dt{};

    while(window.isOpen())
    {
        // EVENT
        sf::Event event{};
        while(window.pollEvent(event))
        {
            if(event.type == sf::Event::KeyPressed){
                if(event.key.code == sf::Keyboard::R){
                    entities.clear();
                }
            }

            if(event.type == sf::Event::KeyReleased){

            }

            if(event.type == sf::Event::MouseButtonPressed){
                if(event.mouseButton.button == sf::Mouse::Button::Left){
                    mousePressed = true;
                }
            }

            if(event.type == sf::Event::MouseButtonReleased){
                if(event.mouseButton.button == sf::Mouse::Button::Left){
                    mousePressed = false;
                }
            }

            if(event.type == sf::Event::Closed){
                window.close();
            }
        }

        // UPDATE
        dt = clock.restart();

        timeSinceFPSUpdate += dt.asMilliseconds();
        if(timeSinceFPSUpdate > 75.f){
            timeSinceFPSUpdate = 0.f;
            fps.setString(std::to_string(static_cast<int>(1.f/dt.asSeconds())));
        }

        for(auto&& e : entities){
            e.move(dt.asSeconds());
        }

        
        Quadtree qRoot{sf::FloatRect{0.f, 0.f, WINDOW_W, WINDOW_H}};
        for(auto&& e : entities){
            e.setFillColor(sf::Color::Blue);
            qRoot.insert(&e);
        }
            
        if(quadtreeDetection){
            for(unsigned int i = 0; i < entities.size(); i++){
                const sf::FloatRect rangeBounds{{entities[i].getCenter().x - 12.f, entities[i].getCenter().y - 12.f}, {24.f, 24.f}};
                entitiesInRange.clear();
                qRoot.queryEntities(entitiesInRange, rangeBounds);
                for(unsigned int j = 0; j < entitiesInRange.size(); j++){
                    if(&entities[i] != entitiesInRange[j]){
                        // Is collide ?
                        if(isCollide(&entities[i], entitiesInRange[j])){
                            entities[i].changeDir();
                            entities[i].setFillColor(sf::Color::Red);
                        }
                    }
                }
            }
        } else {
            for(unsigned int i = 0; i < entities.size(); i++){
                for(unsigned int j = 0; j < entities.size(); j++){
                    if(i != j){
                        if(isCollide(&entities[i], &entities[j])){
                            entities[i].changeDir();
                            entities[i].setFillColor(sf::Color::Red);
                        }
                    }
                }
            }
        }
        

        // DRAW
        window.clear();
        window.draw(qRoot);
        for(auto&& e : entities){
            window.draw(e);
        }
        window.draw(fps);
        window.display();
    }

    return 0;
}
