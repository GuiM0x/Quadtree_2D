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
public:
    Entity(const sf::Vector2f& size, const sf::Vector2f& position);

public:
    sf::Vector2f getCenter() const
    {
        const sf::FloatRect boundary = getGlobalBounds();
        return sf::Vector2f{
            boundary.left + (boundary.width/2.f),
            boundary.top + (boundary.height/2.f)
        };
    }
};

Entity::Entity(const sf::Vector2f& size, const sf::Vector2f& position):
    sf::RectangleShape{size}
{
    setPosition(position);
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
    Quadtree(const sf::FloatRect& boundary);

public:
    bool insert(const Entity* entity);
    void queryEntities(std::vector<Entity>& entitiesInrRange, const sf::FloatRect& range);

private:
    void subdivide();
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;

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
    m_rect.setOutlineColor(sf::Color::Black);
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

void Quadtree::queryEntities(std::vector<Entity>& entitiesInRange, const sf::FloatRect& range)
{
    if(!m_boundary.intersects(range)) return;

    for(auto&& e : m_entities){
        if(range.contains(e->getCenter())){
            entitiesInRange.emplace_back(*e);
            entitiesInRange.back().setFillColor(sf::Color::Red);
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

void createEntity(std::vector<Entity>& entities, const sf::RenderWindow& window)
{
    sf::Vector2i mPos = sf::Mouse::getPosition(window);
    float x = rollTheDice(mPos.x - 32.f, mPos.x + 32.f);
    float y = rollTheDice(mPos.y - 32.f, mPos.y + 32.f);
    Entity e{{4.f, 4.f}, {x, y}};
    e.setFillColor(sf::Color::Blue);
    entities.emplace_back(e);
}

/*
====================================================================
            MAIN
====================================================================
*/
int main()
{
    const unsigned int WINDOW_W = 1600;
    const unsigned int WINDOW_H = 900;

    sf::RenderWindow window{{WINDOW_W, WINDOW_H}, "Quadtree 2D Collision"};
    std::vector<Entity> entities{};
    std::vector<Entity> entitiesInRange{};
    bool mousePressed = false;
    sf::RectangleShape mouseRangeRect{{300.f, 150.f}};
    mouseRangeRect.setFillColor(sf::Color::Transparent);
    mouseRangeRect.setOutlineThickness(2);
    mouseRangeRect.setOutlineColor(sf::Color::Green);

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
        const sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        const sf::FloatRect rangeBounds = mouseRangeRect.getGlobalBounds();
        mouseRangeRect.setPosition(mousePos.x - (rangeBounds.width/2.f), mousePos.y - (rangeBounds.height/2.f));

        if(mousePressed) createEntity(entities, window);

        Quadtree qRoot{sf::FloatRect{0.f, 0.f, WINDOW_W, WINDOW_H}};
        for(auto&& e : entities){
            qRoot.insert(&e);
        }

        entitiesInRange.clear();
        qRoot.queryEntities(entitiesInRange, rangeBounds);

        // DRAW
        window.clear();
        window.draw(qRoot);
        for(auto&& e : entities){
            window.draw(e);
        }
        window.draw(mouseRangeRect);
        for(auto&& e : entitiesInRange){
            window.draw(e);
        }
        window.display();
    }

    return 0;
}
